#pragma once
#include "Pipe.h"
class CPipeServer : public CPipe
{
	friend class CPipe;

public:
	~CPipeServer();

protected:
	CPipeServer();
	HRESULT setup();
};
