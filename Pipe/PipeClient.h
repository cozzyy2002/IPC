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

protected:
};
