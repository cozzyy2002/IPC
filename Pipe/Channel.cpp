#include "stdafx.h"
#include "Channel.h"

IO::IO(Type type, Channel* channel)
	: type(type), channel(channel)
{
	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = hEvent;
}

Channel::Channel(CPipe* pipe)
	: pipe(pipe), isConnected(false)
	, connectIO(IO::Type::Connect, this)
	, receiveIO(IO::Type::Receive, this)
	, sendIO(IO::Type::Send, this)
{
}

void Channel::invalidate()
{
	std::lock_guard<std::mutex> lock(sendMutex);

	isConnected = false;

	onDisconnected = nullptr;
	onCompletedToSend = nullptr;
	onReceived = nullptr;

	readBuffer.Release();
	buffersToSend.clear();
}