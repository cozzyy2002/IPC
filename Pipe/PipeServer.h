#pragma once
#include "Pipe.h"
class CPipeServer : public CPipe
{
	friend class CPipe;

public:
	~CPipeServer();

	HRESULT setup();

protected:
	CPipeServer(int channelCount = 1);
};
