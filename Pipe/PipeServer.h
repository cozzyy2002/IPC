#pragma once
#include "Pipe.h"
class CPipeServer : public CPipe
{
public:
	CPipeServer();
	~CPipeServer();

	HRESULT start(int channelCount = 1);
	HRESULT stop();
	HRESULT disconnect(IChannel* channel);
	HRESULT send(IChannel* channel, IBuffer* iBuffer) { return CPipe::send(channel, iBuffer); }

	std::function <HRESULT(IChannel* channel)> onConnected;
	std::function <HRESULT(IChannel* channel)> onDisconnected;
	std::function <HRESULT(IChannel* channel, IBuffer*)> onCompletedToSend;
	std::function <HRESULT(IChannel* channel, IBuffer*)> onReceived;

protected:
	virtual HRESULT handleConnectedEvent(Channel* channel);
	virtual HRESULT handleErrorEvent(Channel* channel, HRESULT hr);
	virtual HRESULT handleReceivedEvent(Channel* channel, IBuffer* buffer);
	virtual HRESULT handleCompletedToSendEvent(Channel* channel, IBuffer* buffer);
};
