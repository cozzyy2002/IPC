#pragma once
#include "Pipe.h"
class CPipeClient :
	public CPipe
{
public:
	~CPipeClient();

	static HRESULT createInstance(CPipeClient** ppInstance);

protected:
	CPipeClient();
	HRESULT setup();
};
