#pragma once

#include "Pipe.h"

/*
	Internal buffer deinition
*/

struct BufferHeader {
	DWORD userDataSize;
};

/**
	BYTE m_buffer[sizeof(BufferHeader) + size]
	[0]	BufferHeader	<- getHeader()
	:					<- sizeof(BufferHeader)
	[N]	user data		<- getBuffer()
	:					<- getSize()
	[M]
*/
class CBuffer : public CPipe::IBuffer, public CUnknownImpl {
public:
	using buffer_t = std::unique_ptr<BYTE[]>;

	CBuffer(DWORD size, HRESULT& hr);

	virtual HRESULT GetSie(DWORD* pSize);
	virtual HRESULT GetBuffer(BYTE** ppBuffer);

	IUNKNOWN_METHODS;

	// Internal methods
	DWORD getSize() { return m_size; }
	DWORD getTotalSize() { return m_totalSize; }
	BufferHeader* getHeader() { return (BufferHeader*)(m_buffer ? m_buffer.get() : NULL); }
	BYTE* getBuffer() { return m_buffer ? m_buffer.get() + sizeof(BufferHeader) : NULL; }

	static CBuffer* getImpl(IBuffer* i) { return dynamic_cast<CBuffer*>(i); }

protected:
	DWORD m_size;
	DWORD m_totalSize;
	buffer_t m_buffer;
};
