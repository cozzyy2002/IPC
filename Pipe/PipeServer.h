#pragma once
#include "Pipe.h"
class CPipeServer : public CPipe
{
	friend class CPipe;

public:
	~CPipeServer();

protected:
	CPipeServer(int channelCount = 1);
	HRESULT setup();
};
