#include "stdafx.h"
#include "PipeClient.h"
#include "Channel.h"


CPipeClient::CPipeClient() : CPipe(1)
{
}


CPipeClient::~CPipeClient()
{
}

HRESULT CPipeClient::setup()
{
	Channel* channel = (Channel*)m_channels[0].get();
	channel->hPipe.reset(CreateFile(m_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	WIN32_ASSERT(channel->hPipe.isValid());

	// At this point, pipe is connected to server.
	WIN32_ASSERT(SetEvent(channel->connectIO));
	return CPipe::setup();
}
