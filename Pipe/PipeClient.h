#pragma once
#include "Pipe.h"
class CPipeClient : public CPipe
{
public:
	CPipeClient();
	~CPipeClient();

	HRESULT start(int channelCount = 1);
	HRESULT stop();
	HRESULT connect(int ch);
	HRESULT disconnect(int ch);
	HRESULT send(int ch, IBuffer* iBuffer);
	bool isConnected(int ch) const;

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
