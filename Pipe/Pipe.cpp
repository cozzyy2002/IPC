#include "stdafx.h"
#include "Pipe.h"
#include "Buffer.h"
#include <win32/Enum.h>
#include <win32/ComUtils.h>
#include <mutex>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Pipe.Pipe"));

static std::mutex g_sendMutex;

/*static*/ const LPCTSTR CPipe::m_pipeName = _T("\\\\.\\pipe\\CPipe.MasterPipe");

// Check result of overlapped IO.
static HRESULT checkPending(BOOL ret);

CPipe::CPipe()
	: m_isConnected(false)
{
}


CPipe::~CPipe()
{
}

HRESULT CPipe::setup(bool isConnected)
{
	m_thread = std::thread([this, isConnected]() { return mainThread(isConnected); });

	return S_OK;
}

HRESULT CPipe::mainThread(bool isConnected)
{
	// m_isConnected should be false when this thread terminates.
	CSafeValue<bool, false> _isConnected(&m_isConnected);
	m_isConnected = isConnected;

	LPCSTR className = typeid(*this).name();

	HRESULT hr = S_OK;

	if (m_isConnected) {
		WIN32_ASSERT(SetEvent(m_connectIO));
	}

	CBuffer::Header readHeader = { 0 };
	CComPtr<IBuffer> readBuffer;

	// Index of events used as object to be waited in worker thread.
	ENUM(WaitResult, Connected, Received, Sent, Shutdown);

	while (hr == S_OK) {
		// WaitResult                         Connected    Received     Sent      Shutdown
		HANDLE hEvents[WaitResult::COUNT] = { m_connectIO, m_receiveIO, m_sendIO, m_shutdownEvent };
		WaitResult wait((WaitResult::Values)WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE));
		LOG4CPLUS_DEBUG(logger, className << ": Event set: " << wait.toString() << ":" << (int)wait);

		switch (wait) {
		case wait.Connected:	// Connected
			LOG4CPLUS_INFO(logger, className << ": Connected.");
			m_isConnected = true;
			WIN32_ASSERT(ResetEvent(m_connectIO));
			if (onConnected) {
				HR_ASSERT_OK(onConnected());
			}

			// Prepare to receive header
			HR_ASSERT_OK(read(&readHeader, sizeof(readHeader)));
			break;
		case wait.Received:		// Received header or user data.
			{
				DWORD numberOfBytesTransferred = 0;
				WIN32_ASSERT(GetOverlappedResult(m_pipe, &m_receiveIO, &numberOfBytesTransferred, FALSE));
				LOG4CPLUS_DEBUG(logger, "Received " << numberOfBytesTransferred << "byte");
				if(!readBuffer) {
					// Received buffer header.
					HR_ASSERT(sizeof(readHeader) == numberOfBytesTransferred, E_UNEXPECTED);

					// Prepare to receive user data.
					HR_ASSERT_OK(IBuffer::createInstance(readHeader.dataSize, &readBuffer));
					CBuffer* p = CBuffer::getImpl(readBuffer);
					HR_ASSERT_OK(read(p->data, readHeader.dataSize));
				} else {
					// Receied user data.
					CBuffer* p = CBuffer::getImpl(readBuffer);
					HR_ASSERT(p->header->dataSize == numberOfBytesTransferred, E_UNEXPECTED);

					if (onReceived) {
						HR_ASSERT_OK(onReceived(readBuffer));
					}

					// Prepare to receive next header
					readBuffer.Release();
					HR_ASSERT_OK(read(&readHeader, sizeof(readHeader)));
				}
			}
			break;
		case wait.Sent:			// Complete to send data.
			{
				std::lock_guard<std::mutex> lock(g_sendMutex);

				HR_ASSERT(0 <= m_buffersToSend.size(), E_UNEXPECTED);
				if (onCompletedToSend) {
					IBuffer* buffer = m_buffersToSend.front();
					HR_ASSERT_OK(onCompletedToSend(buffer));
				}

				// Remove current buffer and send next buffer if exist.
				m_buffersToSend.pop_front();
				if (0 < m_buffersToSend.size()) {
					HR_ASSERT_OK(write(m_buffersToSend.front()));
				} else {
					WIN32_ASSERT(ResetEvent(m_sendIO));
				}
			}
			break;
		case wait.Shutdown:		// Shutdown event
			hr = S_SHUTDOWN;
			break;
		default:
			// Always fails.
			WIN32_ASSERT(wait < ARRAYSIZE(hEvents));
			break;
		}
	}

	return hr;
}

HRESULT CPipe::shutdown()
{
	HRESULT hr = S_OK;
	if (m_shutdownEvent.isValid()) {
		hr = WIN32_EXPECT(SetEvent(m_shutdownEvent));
		if (SUCCEEDED(hr)) {
			m_thread.join();
		}
	}
	return hr;
}

HRESULT CPipe::send(IBuffer* iBuffer)
{
	HR_ASSERT(m_isConnected, E_ILLEGAL_METHOD_CALL);

	HRESULT hr = S_OK;

	std::lock_guard<std::mutex> lock(g_sendMutex);

	m_buffersToSend.push_back(iBuffer);
	if (m_buffersToSend.size() == 1) {
		hr = write(iBuffer);
	}

	return hr;
}

HRESULT CPipe::read(void* buffer, DWORD size)
{
	HRESULT hr = checkPending(ReadFile(m_pipe, buffer, size, NULL, &m_receiveIO));
	LOG4CPLUS_DEBUG(logger, "Reading " << size << " byte. " << (hr == S_OK ? "Done." : "Pending."));
	return hr;
}

HRESULT CPipe::write(IBuffer* iBuffer)
{
	CBuffer* buffer = CBuffer::getImpl(iBuffer);
	HRESULT hr = checkPending(WriteFile(m_pipe, buffer->header, buffer->header->totalSize, NULL, &m_sendIO));
	LOG4CPLUS_DEBUG(logger, "Writing " << buffer->header->totalSize << " byte. " << (hr == S_OK ? "Done." : "Pending."));
	return hr;
}

HRESULT checkPending(BOOL ret)
{
	HRESULT hr = S_OK;

	if (!ret) {
		DWORD error = GetLastError();
		hr = (error == ERROR_IO_PENDING) ? S_FALSE : HRESULT_FROM_WIN32(error);
	}
	return hr;
}

CPipe::IO::IO()
{
	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = hEvent;
}
