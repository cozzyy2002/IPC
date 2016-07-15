#include "stdafx.h"
#include "PipeClient.h"


CPipeClient::CPipeClient()
{
}


CPipeClient::~CPipeClient()
{
}

HRESULT CPipeClient::setup()
{
	m_pipe.reset(CreateFile(m_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	WIN32_ASSERT(m_pipe.isValid());

	// At this point, pipe is connected to server.
	bool isConnected = true;
	return CPipe::setup(isConnected);
}
