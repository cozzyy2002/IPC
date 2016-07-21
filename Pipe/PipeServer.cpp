#include "stdafx.h"
#include "PipeServer.h"
#include "Channel.h"

#include <win32/ComUtils.h>

#include <new>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Pipe.PipeServer"));

CPipeServer::CPipeServer(int channelCount /*= 1*/) : CPipe(channelCount)
{
}

HRESULT CPipeServer::setup()
{
	for (channels_t::iterator ch = m_channels.begin(); ch != m_channels.end(); ch++) {
		Channel* channel = (Channel*)ch->get();

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
			channel->isConnected = true;
			break;
		default:
			WIN32_FAIL(ConnectNamedPipe(), error);
		}
	};

	// Server is waiting for client to connect.
	return CPipe::setup();
}

CPipeServer::~CPipeServer()
{
}
