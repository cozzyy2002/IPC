#pragma once
#include "Pipe.h"
class CPipeServer : public CPipe
{
public:
	CPipeServer();
	~CPipeServer();

	HRESULT start(int channelCount = 1);
	HRESULT stop();
	HRESULT disconnect(int ch);
	HRESULT send(int ch, IBuffer* iBuffer);

	std::function <HRESULT(int ch)> onConnected;
	std::function <HRESULT(int ch)> onDisconnected;
	std::function <HRESULT(int ch, IBuffer*)> onCompletedToSend;
	std::function <HRESULT(int ch, IBuffer*)> onReceived;

protected:
	virtual HRESULT handleConnectedEvent(Channel* channel);
	virtual HRESULT handleErrorEvent(Channel* channel, HRESULT hr);
	virtual HRESULT handleReceivedEvent(Channel* channel, IBuffer* buffer);
	virtual HRESULT handleCompletedToSendEvent(Channel* channel, IBuffer* buffer);
};
