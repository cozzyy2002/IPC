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

protected:
};
