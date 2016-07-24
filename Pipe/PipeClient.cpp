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

	Channel* channel = m_channels[0].get();
	channel->hPipe.reset(CreateFile(m_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	WIN32_ASSERT(channel->hPipe.isValid());

	// At this point, pipe is connected to server.
	WIN32_ASSERT(SetEvent(channel->connectIO));

	return CPipe::start();
}

HRESULT CPipeClient::disconnect()
{
	return CPipe::stop();
}

HRESULT CPipeClient::send(IBuffer* iBuffer)
{
	Channel* channel = m_channels[0].get();

	return CPipe::send(channel, iBuffer);
}
