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

protected:
};
