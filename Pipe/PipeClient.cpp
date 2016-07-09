#include "stdafx.h"
#include "PipeClient.h"


CPipeClient::CPipeClient()
{
}


CPipeClient::~CPipeClient()
{
}

HRESULT CPipeClient::createInstance(CPipeClient ** ppInstance)
{
	HR_ASSERT(ppInstance, E_POINTER);
	*ppInstance = NULL;

	CPipeClient* pThis = new(std::nothrow) CPipeClient();
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

HRESULT CPipeClient::setup()
{
	m_pipe.reset(CreateFile(m_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	WIN32_ASSERT(m_pipe.isValid());

	m_isConnected = true;
	return CPipe::setup();
}
