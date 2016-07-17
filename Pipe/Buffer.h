#pragma once

#include "Pipe.h"

/*
	Internal buffer deinition
*/

class CBuffer : public CPipe::IBuffer, public CUnknownImpl {
public:
	struct Header {
		DWORD totalSize;
		DWORD dataSize;
	};

	Header* header;		// Header address = Top of allocated buffer.
	void* data;			// Data address = Next byte of header.

	CBuffer(DWORD size, HRESULT& hr);

	virtual HRESULT GetSie(DWORD* pSize);
	virtual HRESULT GetBuffer(void** ppBuffer);

	IUNKNOWN_METHODS;

	static CBuffer* getImpl(IBuffer* iBuffer) { return dynamic_cast<CBuffer*>(iBuffer); }

protected:
	std::unique_ptr<BYTE[]> m_buffer;
};
