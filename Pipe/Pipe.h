#pragma once

#include <win32/Win32Utils.h>
#include <vector>
#include <thread>

#define S_PIPE_SHUTDOWN S_FALSE

struct Channel;

class CPipe
{
	friend struct Channel;
public:
	class IBuffer : public IUnknown {
	public:
		virtual HRESULT GetSize(DWORD* pSize) PURE;
		virtual HRESULT GetBuffer(void** pBuffer) PURE;

		static HRESULT createInstance(DWORD size, IBuffer** ppInstance);
		static HRESULT createInstance(DWORD size, const void* data, IBuffer** ppInstance);
	};

	// Opaque channel to identify pipe channel
	struct IChannel {
		virtual ~IChannel() {}

		virtual bool isConnected() const PURE;

		// Per channel callbacks
		// If the callback return S_OK, server's callback is invoked.
		// The callback may return S_FALSE to avoid server's callback to be invoked.
		// Client doen't call per channel callbacks.
		std::function <HRESULT()> onDisconnected;
		std::function <HRESULT(IBuffer*)> onCompletedToSend;
		std::function <HRESULT(IBuffer*)> onReceived;
	};

protected:
	// Header of data to send or to be received.
	struct DataHeader {
		DWORD size;			// Byte size of following data.
	};

	typedef std::vector<std::unique_ptr<Channel>> channels_t;

	CPipe();
	virtual ~CPipe();

	HRESULT setup(int channelCount);
	HRESULT start();
	HRESULT stop();
	HRESULT send(Channel* channel, IBuffer* iBuffer);
	HRESULT send(IChannel* channel, IBuffer* iBuffer);

#pragma region Event handlers implemented by sub classes
	virtual HRESULT handleConnectedEvent(Channel* channel) { return S_OK; }
	virtual HRESULT handleErrorEvent(Channel* channel, HRESULT hr) { return S_OK; }
	virtual HRESULT handleReceivedEvent(Channel* channel, IBuffer* buffer) { return S_OK; }
	virtual HRESULT handleCompletedToSendEvent(Channel* channel, IBuffer* buffer) { return S_OK; }
#pragma endregion

	HRESULT onConnected(Channel* channel);
	HRESULT onReceived(Channel* channel);
	HRESULT onCompletedToSend(Channel* channel);

	HRESULT mainThread();
	HRESULT onExitMainThread();
	HRESULT read(Channel* channel, void* buffer, DWORD size);
	HRESULT write(Channel* channel, IBuffer* iBuffer);

	HRESULT getChannel(IChannel* iChannel, Channel** pChannel) const;

	static const LPCTSTR m_pipeName;
	channels_t m_channels;
	CSafeEventHandle m_shutdownEvent;
	std::thread m_thread;

	template<class T>
	struct MainThreadDeleter {
		void operator()(T* target) { target->onExitMainThread(); }
	};
};
