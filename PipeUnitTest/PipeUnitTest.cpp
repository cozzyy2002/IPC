// PipeUnitTest.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <log4cplus/configurator.h>
#include <string>
#include <map>

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Test.PipeUnitTest"));

int main(int argc, TCHAR* argv[])
{
	log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("log4cplus.properties"));

	::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include<PipeServer.h>
#include<PipeClient.h>

#include <memory>

using namespace testing;

class PipeTest : public Test {
public:
	typedef std::vector<BYTE> data_t;

	std::unique_ptr<CPipeServer> server;
	std::unique_ptr<CPipeClient> client;
	CPipe::IChannel* serverChannel;

	PipeTest() : serverChannel(NULL) {}

	void SetUp() {
		server.reset(new CPipeServer());
		client.reset(new CPipeClient());
	}
	void TearDown() {
		EXPECT_HRESULT_SUCCEEDED(client->disconnect());
		EXPECT_HRESULT_SUCCEEDED(server->stop());
		EXPECT_FALSE(client->isConnected());
		if(serverChannel) EXPECT_FALSE(serverChannel->isConnected());
	}

	void connectAndWait(int serverChannelCount = 1);
	HRESULT bufferToString(CPipe::IBuffer* buffer, std::string& str);
};

void PipeTest::connectAndWait(int serverChannelCount /*= 1*/)
{
	CSafeEventHandle  hServerEvent(FALSE), hClientEvent(FALSE);
	HANDLE hEvents[] = { hServerEvent, hClientEvent };

	server->onConnected = [&](CPipe::IChannel* channel)
	{
		serverChannel = channel;
		SetEvent(hServerEvent);
		return S_OK;
	};
	client->onConnected = [&]()
	{
		SetEvent(hClientEvent);
		return S_OK;
	};

	ASSERT_HRESULT_SUCCEEDED(server->start(serverChannelCount));
	ASSERT_HRESULT_SUCCEEDED(client->connect());

	ASSERT_LT(WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, TRUE, 100), ARRAYSIZE(hEvents));

	ASSERT_TRUE(serverChannel);
	EXPECT_TRUE(serverChannel->isConnected());
	EXPECT_TRUE(client->isConnected());
}

// Convert Received data in IBuffer object to string.
HRESULT PipeTest::bufferToString(CPipe::IBuffer * buffer, std::string & str)
{
	char* data;
	HR_ASSERT_OK(buffer->GetBuffer((void**)&data));
	DWORD size;
	HR_ASSERT_OK(buffer->GetSie(&size));

	str.assign(data, size - 1);	// Exclude last '\0'

	return S_OK;
}

TEST_F(PipeTest, normal)
{
	connectAndWait();

	CSafeEventHandle  hServerEvent(FALSE), hClientEvent(FALSE);
	HANDLE hEvents[] = { hServerEvent, hClientEvent };

	data_t dataToSend{ 1, 2, 3, 0, 5 };
	data_t receivedData;
	server->onReceived = [&](CPipe::IChannel* channel, CPipe::IBuffer* buffer)
	{
		BYTE* data;
		buffer->GetBuffer((void**)&data);
		DWORD size;
		buffer->GetSie(&size);
		receivedData = std::vector<BYTE>(data, data + size);
		SetEvent(hServerEvent);
		return S_OK;
	};

	CPipe::IBuffer* bufferSent = NULL;
	client->onCompletedToSend = [&](CPipe::IBuffer* buffer)
	{
		bufferSent = buffer;
		SetEvent(hClientEvent);
		return S_OK;
	};

	CComPtr<CPipe::IBuffer> buffer;
	ASSERT_HRESULT_SUCCEEDED(CPipe::IBuffer::createInstance(dataToSend.size(), dataToSend.data(), &buffer));
	ASSERT_HRESULT_SUCCEEDED(client->send(buffer));
	WIN32_EXPECT(WaitForMultipleObjects(2, hEvents, TRUE, 1000));

	EXPECT_EQ(dataToSend, receivedData);
	EXPECT_EQ(buffer, bufferSent);
}

TEST_F(PipeTest, MultiData)
{
	connectAndWait();

	CSafeEventHandle  hServerEvent(FALSE), hClientEvent(FALSE);
	HANDLE hEvents[] = { hServerEvent, hClientEvent };

	std::vector<std::string> datasToSend{
		"wstring: A type that describes a secialization of the temlate class basic_staring with elements of type wchar_t.",
		"Writing a Mutithireaded Win32 Program",
		"Sharing Common Resources Between Threads.",
		"mutable std::mutex m;",
		"std::lock_guard<std::mutex> g(m); // lock mutex",
		"項目16: const メンバ関数はスレッドセーフにする",
	};

	// Map of received data
	// Key: received data
	// Value: received count. 1 is expected.
	std::map<std::string, int> datasReceived;

	server->onReceived = [&](CPipe::IChannel* channel, CPipe::IBuffer* buffer)
	{
		std::string str;
		HR_ASSERT_OK(bufferToString(buffer, str));
		std::map<std::string, int>::iterator i = datasReceived.find(str);
		LOG4CPLUS_INFO(logger, "Received: '" << str.c_str() << "'");
		if (i == datasReceived.end()) {
			datasReceived[str] = 1;
		} else {
			i->second++;
		}
		return S_OK;
	};

	std::for_each(datasToSend.begin(), datasToSend.end(), [&](const std::string& str)
	{
		// Send string including terminating zero.
		CComPtr<CPipe::IBuffer> buffer;
		ASSERT_HRESULT_SUCCEEDED(CPipe::IBuffer::createInstance(str.size() + 1, str.c_str(), &buffer));
		ASSERT_HRESULT_SUCCEEDED(client->send(buffer));
	});

	Sleep(1000);

	ASSERT_EQ(datasToSend.size(), datasReceived.size());

	// All datas should be received once.
	std::for_each(datasToSend.begin(), datasToSend.end(), [&](const std::string& str)
	{
		std::map<std::string, int>::iterator i = datasReceived.find(str);
		ASSERT_NE(i, datasReceived.end());
		EXPECT_EQ(i->second, 1);
	});
}
