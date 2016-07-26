#include "stdafx.h"
#include "Buffer.h"

/*static*/ HRESULT CPipe::IBuffer::createInstance(DWORD size, IBuffer** ppInstance)
{
	HR_ASSERT(0 < size, E_INVALIDARG);
	HR_ASSERT(ppInstance, E_POINTER);
	*ppInstance = NULL;

	HRESULT hr;
	CComPtr<CBuffer> instance(new(std::nothrow) CBuffer(size, hr));
	HR_ASSERT(instance, E_OUTOFMEMORY);
	if (FAILED(hr)) return hr;

	*ppInstance = (IBuffer*)instance.Detach();
	return S_OK;
}

/*static*/ HRESULT CPipe::IBuffer::createInstance(DWORD size, const void* data, IBuffer** ppInstance)
{
	HRESULT hr = createInstance(size, ppInstance);
	if (SUCCEEDED(hr)) {
		CBuffer* buffer = CBuffer::getImpl(*ppInstance);
		CopyMemory(buffer->data, data, size);
	}
	return hr;
}

/*
	Constructor.
	Allocate buffer whose size is sum of header and data.
*/
CBuffer::CBuffer(DWORD size, HRESULT& hr)
	: header(NULL), data(NULL)
{
	DWORD totalSize = sizeof(Header) + size;
	m_buffer.reset(new(std::nothrow) BYTE[totalSize]);
	hr = HR_EXPECT(m_buffer, E_OUTOFMEMORY);
	if (SUCCEEDED(hr)) {
		header = (Header*)m_buffer.get();
		header->totalSize = totalSize;
		header->dataSize = size;
		data = &m_buffer[sizeof(Header)];
	}
}

HRESULT CBuffer::GetSize(DWORD* pSize)
{
	HR_ASSERT(pSize, E_POINTER);
	*pSize = header->dataSize;
	return S_OK;
}

HRESULT CBuffer::GetBuffer(void** ppBuffer)
{
	HR_ASSERT(ppBuffer, E_POINTER);
	*ppBuffer = data;
	return S_OK;
}
