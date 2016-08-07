#include "stdafx.h"
#include "PipeClient.h"
#include "Channel.h"

CPipeClient::CPipeClient()
{
}


CPipeClient::~CPipeClient()
{
}

HRESULT CPipeClient::start(int channelCount /*= 1*/)
{
	HR_ASSERT_OK(CPipe::setup(channelCount));
	HR_ASSERT_OK(CPipe::start());
	return S_OK;
}

HRESULT CPipeClient::stop()
{
	return CPipe::stop();
}

HRESULT CPipeClient::connect(int ch)
{
	HR_ASSERT_OK(CPipe::setup(ch + 1));

	HR_ASSERT(ch < (int)m_channels.size(), E_INVALIDARG);
	HR_ASSERT(!m_channels[ch]->isConnected(), E_ILLEGAL_METHOD_CALL);

	Channel* channel = m_channels[ch].get();
	channel->hPipe.reset(CreateFile(m_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	WIN32_ASSERT(channel->hPipe.isValid());

	// At this point, pipe is connected to server.
	WIN32_ASSERT(SetEvent(channel->connectIO));

	HR_ASSERT_OK(CPipe::start());
	return S_OK;
}

HRESULT CPipeClient::disconnect(int ch)
{
	HR_ASSERT(ch < (int)m_channels.size(), E_INVALIDARG);
	HR_ASSERT(m_channels[ch]->isConnected(), E_ILLEGAL_METHOD_CALL);

	WIN32_ASSERT(CloseHandle(m_channels[ch]->hPipe));

	if (onDisconnected) {
		HR_ASSERT_OK(onDisconnected(ch));
	}

	return S_OK;
}

HRESULT CPipeClient::send(int ch, IBuffer* iBuffer)
{
	HR_ASSERT(isConnected(ch), E_ILLEGAL_METHOD_CALL);
	Channel* channel = m_channels.begin()->get();

	return CPipe::send(ch, iBuffer);
}

bool CPipeClient::isConnected(int ch) const
{
	if (m_channels.empty()) return false;

	return m_channels[ch].get()->isConnected();
}

HRESULT CPipeClient::handleConnectedEvent(Channel* channel)
{
	if (onConnected) {
		HR_ASSERT_OK(onConnected(channel->index));
	}

	return S_OK;
}

HRESULT CPipeClient::handleErrorEvent(Channel* channel, HRESULT hr)
{
	if (channel->isConnected()) {
		hr = disconnect(channel->index);
	} else {
		return hr;
	}

	return S_OK;
}

HRESULT CPipeClient::handleReceivedEvent(Channel* channel, IBuffer* buffer)
{
	if (onReceived) {
		HR_ASSERT_OK(onReceived(channel->index, buffer));
	}

	return S_OK;
}

HRESULT CPipeClient::handleCompletedToSendEvent(Channel* channel, IBuffer* buffer)
{
	if (onCompletedToSend) {
		HR_ASSERT_OK(onCompletedToSend(channel->index, buffer));
	}

	return S_OK;
}

