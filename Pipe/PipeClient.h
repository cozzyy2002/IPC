#pragma once
#include "Pipe.h"
class CPipeClient : public CPipe
{
public:
	CPipeClient();
	~CPipeClient();

	HRESULT connect();
	HRESULT disconnect();

	HRESULT send(IBuffer* iBuffer);
	bool isConnected() const;

	std::function <HRESULT()> onConnected;
	std::function <HRESULT()> onDisconnected;
	std::function <HRESULT(IBuffer*)> onCompletedToSend;
	std::function <HRESULT(IBuffer*)> onReceived;

protected:
	virtual HRESULT handleConnectedEvent(Channel* channel);
	virtual HRESULT handleErrorEvent(Channel* channel, HRESULT hr);
	virtual HRESULT handleReceivedEvent(Channel* channel, IBuffer* buffer);
	virtual HRESULT handleCompletedToSendEvent(Channel* channel, IBuffer* buffer);
};
