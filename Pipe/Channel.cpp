#include "stdafx.h"
#include "Channel.h"

IO::IO(Type type, Channel* channel)
	: type(type), channel(channel)
{
	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = hEvent;
}

Channel::Channel(CPipe* pipe, int index)
	: pipe(pipe), index(index), m_isConnected(false)
	, connectIO(IO::Type::Connect, this)
	, receiveIO(IO::Type::Receive, this)
	, sendIO(IO::Type::Send, this)
{
}

void Channel::invalidate()
{
	std::lock_guard<std::mutex> lock(sendMutex);

	m_isConnected = false;

	onDisconnected = nullptr;
	onCompletedToSend = nullptr;
	onReceived = nullptr;

	readBuffer.Release();
	buffersToSend.clear();
}
