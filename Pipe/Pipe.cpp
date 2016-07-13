#include "stdafx.h"
#include "Pipe.h"
#include <win32/Enum.h>

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
ENUM(WaitResult, Received, Sent, Shutdown);

HRESULT CPipe::setup()
{
	m_shutdownEvent.reset(CreateEvent(NULL, TRUE, FALSE, NULL));
	WIN32_ASSERT(m_shutdownEvent.isValid());

	m_thread = std::thread([this]() -> HRESULT
	{
		// m_isConnected should be false when this thread terminates.
		CSafeValue<bool, false> isConnected(&m_isConnected);

		LPCSTR className = typeid(*this).name();

		HRESULT hr = S_OK;

		if(m_isConnected) {
			WIN32_ASSERT(SetEvent(m_receiveIO));
			m_isConnected = false;
		}

		while (hr == S_OK) {
			// WaitResult                         Received            Sent             Shutdown
			HANDLE hEvents[WaitResult::COUNT] = { m_receiveIO, m_sendIO, m_shutdownEvent };
			WaitResult wait((WaitResult::Values)WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE));
			LOG4CPLUS_DEBUG(logger, className << ": Event set: " << wait.toString() << ":" << (int)wait);

			switch (wait) {
			case wait.Received:		// Connected or received data.
				if (!m_isConnected) {
					// Connected.
					LOG4CPLUS_INFO(logger, className << ": Connected.");
					m_isConnected = true;
					if (onConnected) {
						HR_ASSERT_OK(onConnected());
					}
				} else {
					// Recived data.
					hr = HR_EXPECT_OK(receiveData());
				}
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
			WIN32_ASSERT(ResetEvent(hEvents[wait]));
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
	hEvent.reset(::CreateEvent(NULL, TRUE, FALSE, NULL));
	WIN32_EXPECT(hEvent.isValid());

	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = hEvent;
}
