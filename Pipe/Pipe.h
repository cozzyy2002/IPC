#pragma once

#include <win32/Win32Utils.h>
#include <vector>
#include <deque>
#include <thread>

#define S_SHUTDOWN S_FALSE

class CPipe
{
public:
	typedef std::vector<BYTE> Data;

	class IBuffer : public IUnknown {
	public:
		virtual HRESULT GetSie(DWORD* pSize) PURE;
		virtual HRESULT GetBuffer(BYTE** pBuffer) PURE;

		static HRESULT createInstance(DWORD size, IBuffer** ppInstance);
	};

	CPipe();
	virtual ~CPipe();

	template<class T>
	static HRESULT createInstance(std::unique_ptr<T>& ptr);
	template<class T>
	static HRESULT createInstance(T** pp);

	HRESULT shutdown();

	HRESULT send(const Data& data) { return send(data.data(), data.size()); }
	HRESULT send(const BYTE* data, size_t size);
	HRESULT send(IBuffer* iBuffer);

	inline bool isConnected() const { return m_isConnected; }

	std::function <HRESULT()> onConnected;
	std::function <HRESULT()> onCompletedToSend;
	std::function <HRESULT(const Data& data)> onReceived;

protected:
	// Structure used by overlapped IO
	struct IO {
		IO();

		OVERLAPPED* operator&() { return &ov; }
		operator HANDLE() { return ov.hEvent; }

	protected:
		OVERLAPPED ov;
		CSafeEventHandle hEvent;
	};

	// Header of data to send or to be received.
	struct DataHeader {
		DWORD size;			// Byte size of following data.
	};

	HRESULT setup(bool isConnected);
	HRESULT receiveData();
	HRESULT write(IBuffer* iBuffer);

	static const LPCTSTR m_pipeName;
	CSafeHandle m_pipe;
	CSafeEventHandle m_shutdownEvent;
	IO m_connectIO;		// IO structure used when connect
	IO m_receiveIO;		// IO structure used when receive data
	IO m_sendIO;		// IO structure used when complete to send data
	bool m_isConnected;
	DataHeader m_dataHeader;
	std::thread m_thread;

	/**
		IBuffer pointer queue to send.
		Top of IBUffer in queue is currently in progress sending operation.
	*/
	std::deque<CComPtr<IBuffer>> m_buffersToSend;
};

template<class T>
inline HRESULT CPipe::createInstance(std::unique_ptr<T>& ptr)
{
	ptr.reset(new(std::nothrow) T());
	HR_ASSERT(ptr, E_OUTOFMEMORY);

	HRESULT hr = ptr->setup();
	if (FAILED(hr)) {
		ptr.reset();
	}
	return hr;
}

template<class T>
inline HRESULT CPipe::createInstance(T ** pp)
{
	HR_ASSERT(pp, E_POINTER);

	std::unique_ptr<T> ptr;
	HRESULT hr = createInstance(ptr);
	*pp = ptr.release();
	return hr;
}
