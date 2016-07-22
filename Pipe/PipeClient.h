#pragma once
#include "Pipe.h"
class CPipeClient : public CPipe
{
	friend class CPipe;

public:
	~CPipeClient();

	HRESULT setup();

protected:
	CPipeClient();
};
