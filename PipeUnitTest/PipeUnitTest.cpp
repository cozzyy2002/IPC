// PipeUnitTest.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <log4cplus/configurator.h>

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
	std::unique_ptr<CPipeServer> server;
	std::unique_ptr<CPipeClient> client;

	void SetUp() {}
	void TearDown() {
		EXPECT_HRESULT_SUCCEEDED(client->shutdown());
		EXPECT_HRESULT_SUCCEEDED(server->shutdown());
		EXPECT_FALSE(client->isConnected());
		EXPECT_FALSE(server->isConnected());
	}
};

TEST_F(PipeTest, normal)
{
	ASSERT_HRESULT_SUCCEEDED(CPipe::createInstance(server));
	ASSERT_HRESULT_SUCCEEDED(CPipe::createInstance(client));

	// NOTE: When server connects, client does not connect yet.
	CSafeEventHandle  hServerEvent(FALSE), hClientEvent(FALSE);
	HANDLE hEvents[] = { hServerEvent, hClientEvent };

	server->onConnected = [&]()
	{
		SetEvent(hServerEvent);
		return S_OK;
	};
	client->onConnected = [&]()
	{
		SetEvent(hClientEvent);
		return S_OK;
	};
	WIN32_EXPECT(WaitForMultipleObjects(2, hEvents, TRUE, 1000));

	ASSERT_TRUE(server->isConnected());
	ASSERT_TRUE(client->isConnected());

	CPipe::Data dataToSend{ 1, 2, 3, 0, 5 };
	CPipe::Data receivedData;
	server->onReceived = [&](const CPipe::Data& data)
	{
		receivedData = data;
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
	ASSERT_HRESULT_SUCCEEDED(CPipe::IBuffer::createInstance(dataToSend, &buffer));
	ASSERT_HRESULT_SUCCEEDED(client->send(buffer));
	WIN32_EXPECT(WaitForMultipleObjects(2, hEvents, TRUE, 1000));

	EXPECT_EQ(dataToSend, receivedData);
	EXPECT_EQ(buffer, bufferSent);
}
