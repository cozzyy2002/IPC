#pragma once
#include "Pipe.h"
class CPipeClient : public CPipe
{
	friend class CPipe;

public:
	~CPipeClient();

protected:
	CPipeClient();
	HRESULT setup();
};
