#include "stdafx.h"
#include "PipeServer.h"
#include <win32/ComUtils.h>

#include <new>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Pipe.PipeServer"));

CPipeServer::CPipeServer()
{
}

HRESULT CPipeServer::setup()
{
	DWORD dwOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE;
	DWORD dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;
	DWORD nMaxInstanes = 1;
	DWORD nOutBufferSize = 128;
	DWORD nInBufferSize = 128;
	DWORD nDefaultTimOout = 1000;
	m_pipe.reset(::CreateNamedPipe(m_pipeName, dwOpenMode, dwPipeMode, nMaxInstanes, nOutBufferSize, nInBufferSize, nDefaultTimOout, NULL));
	WIN32_ASSERT(m_pipe.isValid());
	HR_ASSERT(!ConnectNamedPipe(m_pipe, &m_receiveIO.ov), E_ABORT);	// ConnetNamedPipe() should return FALSE.
	DWORD error = GetLastError();
	HR_ASSERT(error == ERROR_IO_PENDING, HRESULT_FROM_WIN32(error));

	return CPipe::setup();
}


CPipeServer::~CPipeServer()
{
}

HRESULT CPipeServer::createInstance(CPipeServer ** ppInstance)
{
	HR_ASSERT(ppInstance, E_POINTER);
	*ppInstance = NULL;

	CPipeServer* pThis = new(std::nothrow) CPipeServer();
	HR_ASSERT(pThis, E_OUTOFMEMORY);

	HRESULT hr = pThis->setup();
	if (SUCCEEDED(hr)) {
		// Completed to create object.
		*ppInstance = pThis;
	} else {
		delete pThis;
	}
	return hr;
}
