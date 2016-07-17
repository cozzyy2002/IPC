#include "stdafx.h"
#include "Buffer.h"

HRESULT CPipe::IBuffer::createInstance(DWORD size, IBuffer** ppInstance)
{
	HR_ASSERT(ppInstance, E_POINTER);
	*ppInstance = NULL;

	HRESULT hr;
	CComPtr<CBuffer> instance(new(std::nothrow) CBuffer(size, hr));
	HR_ASSERT(instance, E_OUTOFMEMORY);
	if (FAILED(hr)) return hr;

	*ppInstance = instance.Detach();
	return S_OK;
}

CBuffer::CBuffer(DWORD size, HRESULT& hr) : m_size(size)
{
	m_totalSize = sizeof(BufferHeader) + size;
	m_buffer.reset(new(std::nothrow) CBuffer::buffer_t::element_type[m_totalSize]);
	hr = HR_EXPECT(m_buffer, E_OUTOFMEMORY);
	if (SUCCEEDED(hr)) {
		getHeader()->userDataSize = size;
	}
}

HRESULT CBuffer::GetSie(DWORD * pSize)
{
	HR_ASSERT(pSize, E_POINTER);
	*pSize = getSize();
	return S_OK;
}

HRESULT CBuffer::GetBuffer(BYTE ** ppBuffer)
{
	HR_ASSERT(ppBuffer, E_POINTER);
	*ppBuffer = getBuffer();
	return S_OK;
}
