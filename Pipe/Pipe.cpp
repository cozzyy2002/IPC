#include "stdafx.h"
#include "Pipe.h"
#include "Channel.h"
#include "Buffer.h"
#include <win32/ComUtils.h>
#include <mutex>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Pipe.Pipe"));

/*static*/ const LPCTSTR CPipe::m_pipeName = _T("\\\\.\\pipe\\CPipe.MasterPipe");

// Check result of overlapped IO.
static HRESULT checkPending(BOOL ret);

CPipe::CPipe(int channelCount)
{
	m_channels.resize(channelCount);
	for (int i = 0; i < channelCount; i++) {
		m_channels[i] = std::move(channels_t::value_type(new Channel(this)));
	}
}


CPipe::~CPipe()
{
}

HRESULT CPipe::setup()
{
	m_thread = std::thread([this]() { return mainThread(); });

	return S_OK;
}

HRESULT CPipe::mainThread()
{
	// Register cleanup code.
	std::unique_ptr<CPipe, MainThreadDeleter<CPipe>> _onExit(this);

	LPCSTR className = typeid(*this).name();

	HRESULT hr = S_OK;

	// Events for connect, receive, send and shutdown.
	static const DWORD eventCount = (m_channels.size() * IO::Type::COUNT) + 1;
	std::unique_ptr<HANDLE[]> hEvents(new HANDLE[eventCount]);
	for (size_t i = 0; i < m_channels.size(); i++) {
		int ch = i * IO::Type::COUNT;
		Channel* channel = ((Channel*)m_channels[i].get());
		hEvents[ch + channel->connectIO.type] = channel->connectIO;
		hEvents[ch + channel->receiveIO.type] = channel->receiveIO;
		hEvents[ch + channel->sendIO.type] = channel->sendIO;
	}
	hEvents[eventCount - 1] = m_shutdownEvent;

	while (hr == S_OK) {
		DWORD wait = WaitForMultipleObjects(eventCount, hEvents.get(), FALSE, INFINITE);

		HR_ASSERT(wait != WAIT_TIMEOUT, E_UNEXPECTED);	// Timeout(can not occur)
		WIN32_ASSERT(wait < eventCount);				// Error
		if(wait == (eventCount - 1)) {					// Shutdown event
			LOG4CPLUS_INFO(logger, className << ": mainThread() exits.");
			hr = S_PIPE_SHUTDOWN;
			break;
		}

		DWORD ch = wait / IO::Type::COUNT;
		IO::Type ioType((IO::Type::Values)(wait % IO::Type::COUNT));
		LOG4CPLUS_DEBUG(logger, className << ": Event set: channel=" << ch << ", IO=" << IO::Type::toString(ioType) << ":" << (int)ioType);

		HR_ASSERT(ch < m_channels.size(), E_UNEXPECTED);

		Channel* channel = (Channel*)m_channels[ch].get();
		DWORD numberOfBytesTransferred;

		switch (ioType) {
		case IO::Type::Connect:	// Connected
			LOG4CPLUS_INFO(logger, className << ": Connected. Channel=" << ch);
			channel->m_isConnected = true;
			WIN32_ASSERT(ResetEvent(channel->connectIO));
			if (onConnected) {
				HR_ASSERT_OK(onConnected(channel));
			}

			// Prepare to receive header
			HR_ASSERT_OK(read(channel, &channel->readHeader, sizeof(channel->readHeader)));
			break;

		case IO::Type::Receive:		// Received header or user data.
			WIN32_ASSERT(GetOverlappedResult(channel->hPipe, &channel->receiveIO, &numberOfBytesTransferred, FALSE));
			LOG4CPLUS_DEBUG(logger, "Received " << numberOfBytesTransferred << "byte");
			if(!channel->readBuffer) {
				// Received buffer header.
				HR_ASSERT(sizeof(channel->readHeader) == numberOfBytesTransferred, E_UNEXPECTED);

				// Prepare to receive user data.
				HR_ASSERT_OK(IBuffer::createInstance(channel->readHeader.dataSize, &channel->readBuffer));
				CBuffer* p = CBuffer::getImpl(channel->readBuffer);
				HR_ASSERT_OK(read(channel, p->data, channel->readHeader.dataSize));
			} else {
				// Receied user data.
				CBuffer* p = CBuffer::getImpl(channel->readBuffer);
				HR_ASSERT(p->header->dataSize == numberOfBytesTransferred, E_UNEXPECTED);

				if (onReceived) {
					HR_ASSERT_OK(onReceived(channel, channel->readBuffer));
				}

				// Prepare to receive next header
				channel->readBuffer.Release();
				HR_ASSERT_OK(read(channel, &channel->readHeader, sizeof(channel->readHeader)));
			}
			break;

		case IO::Type::Send:			// Complete to send data.
			{
				std::lock_guard<std::mutex> lock(channel->sendMutex);

				HR_ASSERT(0 <= channel->buffersToSend.size(), E_UNEXPECTED);
				if (onCompletedToSend) {
					IBuffer* buffer = channel->buffersToSend.front();
					HR_ASSERT_OK(onCompletedToSend(channel, buffer));
				}

				// Remove current buffer and send next buffer if exist.
				channel->buffersToSend.pop_front();
				if (0 < channel->buffersToSend.size()) {
					HR_ASSERT_OK(write(channel, channel->buffersToSend.front()));
				} else {
					WIN32_ASSERT(ResetEvent(channel->sendIO));
				}
			}
			break;
		default:
			// Always fails.
			HR_ASSERT(!"IO type is out of range", E_UNEXPECTED);
			break;
		}
	}

	return hr;
}

HRESULT CPipe::onExitMainThread()
{
	for (channels_t::iterator i = m_channels.begin(); i != m_channels.end(); i++) {
		((Channel*)i->get())->invalidate();
	}

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

HRESULT CPipe::send(IChannel* iChannel, IBuffer* iBuffer)
{
	Channel* channel = (Channel*)iChannel;
	std::lock_guard<std::mutex> lock(channel->sendMutex);

	HR_ASSERT(channel->m_isConnected, E_ILLEGAL_METHOD_CALL);

	HRESULT hr = S_OK;

	channel->buffersToSend.push_back(iBuffer);
	if (channel->buffersToSend.size() == 1) {
		hr = write(channel, iBuffer);
	}

	return hr;
}

HRESULT CPipe::read(IChannel* iChannel, void* buffer, DWORD size)
{
	Channel* channel = (Channel*)iChannel;
	HRESULT hr = checkPending(ReadFile(channel->hPipe, buffer, size, NULL, &channel->receiveIO));
	LOG4CPLUS_DEBUG(logger, "Reading " << size << " byte. " << (hr == S_OK ? "Done." : "Pending."));
	return hr;
}

HRESULT CPipe::write(IChannel* iChannel, IBuffer* iBuffer)
{
	Channel* channel = (Channel*)iChannel;
	CBuffer* buffer = CBuffer::getImpl(iBuffer);
	HRESULT hr = checkPending(WriteFile(channel->hPipe, buffer->header, buffer->header->totalSize, NULL, &channel->sendIO));
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
