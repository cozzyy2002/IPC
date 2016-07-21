#pragma once

/*
	Channel implementation.
*/

#include "Pipe.h"
#include "Buffer.h"
#include <win32/Enum.h>
#include <deque>
#include <mutex>

struct Channel;

// Structure used by overlapped IO
struct IO {
	ENUM(Type, Connect, Receive, Send);

	IO(Type type, Channel* channel);

	OVERLAPPED* operator&() { return &ov; }
	operator HANDLE() { return ov.hEvent; }

	Type type;
	Channel* channel;

protected:
	OVERLAPPED ov;
	CSafeEventHandle hEvent;
};

/*
	IChannel implementation.
*/
struct Channel : public CPipe::IChannel {
	CSafeHandle hPipe;	// Pipe handle

	IO connectIO;		// IO structure used when connect
	IO receiveIO;		// IO structure used when receive data
	IO sendIO;			// IO structure used when complete to send data

	bool isConnected;
	virtual HRESULT send(CPipe::IBuffer* buffer) { return pipe->send(this, buffer); }

	CBuffer::Header readHeader;
	CComPtr<CPipe::IBuffer> readBuffer;

	/**
	IBuffer pointer queue to send.
	Top of IBUffer in queue is currently in progress sending operation.
	*/
	std::deque<CComPtr<CPipe::IBuffer>> buffersToSend;

	std::mutex sendMutex;

	Channel(CPipe* pipe);
	void invalidate();

protected:
	CPipe* pipe;
};
