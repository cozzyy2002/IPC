#include "stdafx.h"
#include "Pipe.h"
#include <win32/Enum.h>
#include <win32/ComUtils.h>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Pipe.Pipe"));

/*static*/ const LPCTSTR CPipe::m_pipeName = _T("\\\\.\\pipe\\CPipe.MasterPipe");

CPipe::CPipe()
	: m_isConnected(false)
{
}


CPipe::~CPipe()
{
}

HRESULT CPipe::send(const BYTE * data, size_t size)
{
	HR_ASSERT(m_isConnected, E_ILLEGAL_METHOD_CALL);

	// Send header
	IO headerIO;
	DataHeader header = { size };
	WriteFile(m_pipe, &header, sizeof(header), NULL, &headerIO);
	HANDLE hEvents[] = { headerIO, m_shutdownEvent };
	DWORD wait = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE);
	switch (wait) {
	case 0:
		// Send data
		WriteFile(m_pipe, data, size, NULL, &m_sendIO);
		break;
	case 1:
		return S_SHUTDOWN;
	default:
		// Always fails.
		WIN32_ASSERT(wait < ARRAYSIZE(hEvents));
		break;
	}
	return S_OK;
}

// Index of events used as object to be waited in worker thread.
ENUM(WaitResult, Connected, Received, Sent, Shutdown);

HRESULT CPipe::setup(bool isConnected)
{
	m_thread = std::thread([this, isConnected]() -> HRESULT
	{
		// m_isConnected should be false when this thread terminates.
		CSafeValue<bool, false> _isConnected(&m_isConnected);
		m_isConnected = isConnected;

		LPCSTR className = typeid(*this).name();

		HRESULT hr = S_OK;

		if(m_isConnected) {
			WIN32_ASSERT(SetEvent(m_connectIO));
		}

		while (hr == S_OK) {
			// WaitResult                         Connected    Received     Sent      Shutdown
			HANDLE hEvents[WaitResult::COUNT] = { m_connectIO, m_receiveIO, m_sendIO, m_shutdownEvent };
			WaitResult wait((WaitResult::Values)WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE));
			LOG4CPLUS_DEBUG(logger, className << ": Event set: " << wait.toString() << ":" << (int)wait);

			switch (wait) {
			case wait.Connected:	// Connected
				LOG4CPLUS_INFO(logger, className << ": Connected.");
				m_isConnected = true;
				if (onConnected) {
					HR_ASSERT_OK(onConnected());
				}
				ReadFile(m_pipe, &m_dataHeader, sizeof(m_dataHeader), NULL, &m_receiveIO);
				break;
			case wait.Received:		// received data.
				hr = HR_EXPECT_OK(receiveData());
				ReadFile(m_pipe, &m_dataHeader, sizeof(m_dataHeader), NULL, &m_receiveIO);
				break;
			case wait.Sent:			// Complete to send data.
				if (onCompletedToSend) {
					HR_ASSERT_OK(onCompletedToSend());
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
			if (wait.isValid()) {
				WIN32_ASSERT(ResetEvent(hEvents[wait]));
			}
		}

		return hr;
	});

	return S_OK;
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

HRESULT CPipe::receiveData()
{
	DWORD numberOfBytesTransferred = 0;
	GetOverlappedResult(m_pipe, &m_receiveIO, &numberOfBytesTransferred, FALSE);
	WIN32_ASSERT(sizeof(m_dataHeader) != numberOfBytesTransferred);

	Data data;
	data.resize(m_dataHeader.size);
	ReadFile(m_pipe, data.data(), m_dataHeader.size, NULL, &m_receiveIO);
	HANDLE hEvents[] = { m_receiveIO, m_shutdownEvent };
	DWORD wait = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE);
	switch (wait) {
	case 0:
		if (onReceived) {
			HR_ASSERT_OK(onReceived(data));
		}
		break;
	case 1:
		return S_SHUTDOWN;
	default:
		// Always fails.
		WIN32_ASSERT(wait < ARRAYSIZE(hEvents));
		break;
	}
	return S_OK;
}

CPipe::IO::IO()
{
	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = hEvent;
}
