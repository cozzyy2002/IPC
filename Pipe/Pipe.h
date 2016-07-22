#pragma once

#include <win32/Win32Utils.h>
#include <vector>
#include <thread>

#define S_PIPE_SHUTDOWN S_FALSE

class CPipe
{
public:
	class IBuffer : public IUnknown {
	public:
		virtual HRESULT GetSie(DWORD* pSize) PURE;
		virtual HRESULT GetBuffer(void** pBuffer) PURE;

		static HRESULT createInstance(DWORD size, IBuffer** ppInstance);
		static HRESULT createInstance(DWORD size, const void* data, IBuffer** ppInstance);
	};

	// Opaque channel to identify pipe channel
	struct IChannel {
		virtual ~IChannel() {}

		// Shortcut to CPipe methods
		virtual HRESULT send(IBuffer* iBuffer) PURE;
		virtual bool isConnected() const PURE;

		// Per channel callbacks
		std::function <HRESULT()> onDisconnected;
		std::function <HRESULT(IBuffer*)> onCompletedToSend;
		std::function <HRESULT(IBuffer*)> onReceived;
	};

	CPipe(int channelCount);
	virtual ~CPipe();

	template<class T>
	static HRESULT createInstance(std::unique_ptr<T>& ptr);
	template<class T>
	static HRESULT createInstance(T** pp);

	HRESULT shutdown();

	inline bool isConnected(IChannel* channel) const { return channel->isConnected(); }

	HRESULT send(IChannel* channel, IBuffer* iBuffer);

	std::function <HRESULT(IChannel* channel)> onConnected;
	std::function <HRESULT(IChannel* channel)> onDisconnected;
	std::function <HRESULT(IChannel* channel, IBuffer*)> onCompletedToSend;
	std::function <HRESULT(IChannel* channel, IBuffer*)> onReceived;

protected:
	// Header of data to send or to be received.
	struct DataHeader {
		DWORD size;			// Byte size of following data.
	};

	typedef std::vector<std::unique_ptr<IChannel>> channels_t;

	HRESULT setup();
	HRESULT mainThread();
	HRESULT read(IChannel* channel, void* buffer, DWORD size);
	HRESULT write(IChannel* channel, IBuffer* iBuffer);

	static const LPCTSTR m_pipeName;
	channels_t m_channels;
	CSafeEventHandle m_shutdownEvent;
	std::thread m_thread;
};

template<class T>
inline HRESULT CPipe::createInstance(std::unique_ptr<T>& ptr)
{
	ptr.reset(new(std::nothrow) T());
	HR_ASSERT(ptr, E_OUTOFMEMORY);

	return S_OK;
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
