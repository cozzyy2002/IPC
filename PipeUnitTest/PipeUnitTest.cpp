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
	CPipeServer* _server;
	ASSERT_HRESULT_SUCCEEDED(CPipeServer::createInstance(&_server));
	server.reset(_server);
	CPipeClient* _client;
	ASSERT_HRESULT_SUCCEEDED(CPipeClient::createInstance(&_client));
	client.reset(_client);

	Sleep(100);
	ASSERT_TRUE(server->isConnected());
	ASSERT_TRUE(client->isConnected());

	CPipe::Data dataToSend{ 1, 2, 3, 0, 5 };
	CPipe::Data receivedData;
	server->onReceived = [&](const CPipe::Data& data)
	{
		receivedData = data;
		return S_OK;
	};

	ASSERT_HRESULT_SUCCEEDED(client->send(dataToSend));
	Sleep(100);

	EXPECT_EQ(dataToSend, receivedData);
}
