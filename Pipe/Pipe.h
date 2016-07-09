#pragma once

#include <win32/Win32Utils.h>
#include <vector>
#include <functional>

#define S_SHUTDOWN S_FALSE

class CPipe
{
public:
	typedef std::vector<BYTE> Data;

	CPipe();
	virtual ~CPipe();

	HRESULT send(const Data& data) { send(data.data(), data.size()); }
	HRESULT send(const BYTE* data, size_t size);

	inline bool isConnected() const { return m_isConnected; }

	std::function <HRESULT()> onConnected;
	std::function <HRESULT(const Data& data)> onReceived;
	std::function <HRESULT()> onCompletedToSend;

protected:
	// Structure used by overlapped IO
	struct IO {
		IO();

		OVERLAPPED ov;
		CSafeHandle hEvent;
	};

	// Header of data to send or to be received.
	struct DataHeader {
		DWORD size;			// Byte size of following data.
	};

	HRESULT setup();
	HRESULT shutdown();
	HRESULT receiveData();

	static const LPCTSTR m_pipeName;
	CSafeHandle m_pipe;
	CSafeHandle m_shutdownEvent;
	IO m_receiveIO;		// IO structure used when connect and receive;
	IO m_sendIO;		// IO structure used when send
	bool m_isConnected;
	DataHeader m_dataHeader;
};
