#include "stdafx.h"
#include "PipeServer.h"
#include "Channel.h"

#include <win32/ComUtils.h>

#include <new>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Pipe.PipeServer"));

CPipeServer::CPipeServer()
{
}

CPipeServer::~CPipeServer()
{
}

HRESULT CPipeServer::start(int channelCount /*= 1*/)
{
	HR_ASSERT(0 < channelCount, E_INVALIDARG);

	HR_ASSERT_OK(CPipe::setup(channelCount));

	for (channels_t::iterator ch = m_channels.begin(); ch != m_channels.end(); ch++) {
		Channel* channel = ch->get();

		// Create server pipe instance
		DWORD dwOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
		DWORD dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;
		DWORD nMaxInstanes = m_channels.size();
		DWORD nOutBufferSize = 128;
		DWORD nInBufferSize = 128;
		DWORD nDefaultTimOout = 1000;
		channel->hPipe.reset(::CreateNamedPipe(m_pipeName, dwOpenMode, dwPipeMode, nMaxInstanes, nOutBufferSize, nInBufferSize, nDefaultTimOout, NULL));
		WIN32_ASSERT(channel->hPipe.isValid());

		// Wait for client to connect
		HR_ASSERT(!ConnectNamedPipe(channel->hPipe, &channel->connectIO), E_ABORT);	// ConnetNamedPipe() should return FALSE.
		DWORD error = GetLastError();
		switch (error) {
		case ERROR_IO_PENDING:
			break;
		case ERROR_PIPE_CONNECTED:
			WIN32_ASSERT(SetEvent(channel->connectIO));
			break;
		default:
			WIN32_FAIL(ConnectNamedPipe(), error);
		}
	};

	// Server is waiting for client to connect.
	return CPipe::start();
}

HRESULT CPipeServer::stop()
{
	return CPipe::stop();
}

HRESULT CPipeServer::disconnect(IChannel* iChannel)
{
	return disconnect(iChannel->getId());
}

HRESULT CPipeServer::disconnect(int id)
{
	HR_ASSERT(id < (int)m_channels.size(), E_INVALIDARG);

	Channel* channel = m_channels[id].get();
	HR_ASSERT(channel->isConnected(), E_ILLEGAL_METHOD_CALL);

	channel->invalidate();
	WIN32_ASSERT(DisconnectNamedPipe(channel->hPipe));

	HRESULT hr = S_OK;
	if (channel->onDisconnected) {
		hr = HR_EXPECT_OK(channel->onDisconnected());
	}
	if ((hr == S_OK) && onDisconnected) {
		hr = HR_EXPECT_OK(onDisconnected(channel));
	}

	return hr;
}

HRESULT CPipeServer::send(IChannel* channel, IBuffer* iBuffer)
{
	return send(channel->getId(), iBuffer);
}

HRESULT CPipeServer::send(int id, IBuffer* iBuffer)
{
	HR_ASSERT(id < (int)m_channels.size(), E_INVALIDARG);

	Channel* channel = m_channels[id].get();
	return CPipe::send(channel, iBuffer);
}

HRESULT CPipeServer::handleConnectedEvent(Channel* channel)
{
	if (onConnected) {
		HR_ASSERT_OK(onConnected(channel));
	}

	return S_OK;
}

HRESULT CPipeServer::handleErrorEvent(Channel* channel, HRESULT hr)
{
	if (channel->isConnected()) {
		hr = disconnect(channel);
	} else {
		return hr;
	}

	return S_OK;
}

HRESULT CPipeServer::handleReceivedEvent(Channel* channel, IBuffer* buffer)
{
	HRESULT hr = S_OK;
	if (channel->onReceived) {
		hr = HR_EXPECT_OK(channel->onReceived(buffer));
	}
	if ((hr == S_OK) && onReceived) {
		hr = HR_EXPECT_OK(onReceived(channel, buffer));
	}

	return hr;
}

HRESULT CPipeServer::handleCompletedToSendEvent(Channel* channel, IBuffer* buffer)
{
	HRESULT hr = S_OK;
	if (channel->onCompletedToSend) {
		hr = HR_EXPECT_OK(channel->onCompletedToSend(buffer));
	}
	if ((hr == S_OK) && onCompletedToSend) {
		hr = HR_EXPECT_OK(onCompletedToSend(channel, buffer));
	}

	return hr;
}

