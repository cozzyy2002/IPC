#include "stdafx.h"
#include "Buffer.h"

/*static*/ HRESULT CPipe::IBuffer::createInstance(DWORD size, IBuffer** ppInstance)
{
	HR_ASSERT(ppInstance, E_POINTER);
	*ppInstance = NULL;

	HRESULT hr;
	CComPtr<CBuffer> instance(new(std::nothrow) CBuffer(size, hr));
	HR_ASSERT(instance, E_OUTOFMEMORY);
	if (FAILED(hr)) return hr;

	*ppInstance = (IBuffer*)instance.Detach();
	return S_OK;
}

/*static*/ HRESULT CPipe::IBuffer::createInstance(DWORD size, const BYTE* data, IBuffer** ppInstance)
{
	HRESULT hr = createInstance(size, ppInstance);
	if (SUCCEEDED(hr)) {
		CBuffer* buffer = CBuffer::getImpl(*ppInstance);
		CopyMemory(buffer->getBuffer(), data, size);
	}
	return hr;
}

/*static*/ HRESULT CPipe::IBuffer::createInstance(const std::vector<BYTE> data, IBuffer** ppInstance)
{
	return createInstance(data.size(), data.data(), ppInstance);
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
