#include "stdafx.h"
#include "PipeClient.h"
#include "Channel.h"


CPipeClient::CPipeClient()
{
}


CPipeClient::~CPipeClient()
{
}

HRESULT CPipeClient::connect()
{
	HR_ASSERT_OK(CPipe::setup(1));

	Channel* channel = m_channels.begin()->get();
	channel->hPipe.reset(CreateFile(m_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	WIN32_ASSERT(channel->hPipe.isValid());

	// At this point, pipe is connected to server.
	WIN32_ASSERT(SetEvent(channel->connectIO));

	return CPipe::start();
}

HRESULT CPipeClient::disconnect()
{
	HR_ASSERT_OK(CPipe::stop());

	if (onDisconnected) {
		HR_ASSERT_OK(onDisconnected());
	}

	return S_OK;
}

HRESULT CPipeClient::send(IBuffer* iBuffer)
{
	HR_ASSERT(isConnected(), E_ILLEGAL_METHOD_CALL);
	Channel* channel = m_channels.begin()->get();

	return CPipe::send(channel, iBuffer);
}

bool CPipeClient::isConnected() const
{
	if (m_channels.empty()) return false;

	return m_channels.begin()->get()->isConnected();
}

HRESULT CPipeClient::handleConnectedEvent(Channel* channel)
{
	if (onConnected) {
		HR_ASSERT_OK(onConnected());
	}

	return S_OK;
}

HRESULT CPipeClient::handleErrorEvent(Channel* channel, HRESULT hr)
{
	if (channel->isConnected()) {
		hr = disconnect();
	} else {
		return hr;
	}

	return S_OK;
}

HRESULT CPipeClient::handleReceivedEvent(Channel* channel, IBuffer* buffer)
{
	if (onReceived) {
		HR_ASSERT_OK(onReceived(buffer));
	}

	return S_OK;
}

HRESULT CPipeClient::handleCompletedToSendEvent(Channel* channel, IBuffer* buffer)
{
	if (onCompletedToSend) {
		HR_ASSERT_OK(onCompletedToSend(buffer));
	}

	return S_OK;
}

