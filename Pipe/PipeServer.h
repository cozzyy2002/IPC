#pragma once
#include "Pipe.h"
class CPipeServer :
	public CPipe
{
public:
	~CPipeServer();

	static HRESULT createInstance(CPipeServer** ppInstance);

protected:
	CPipeServer();
	HRESULT setup();
};
