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

TEST(Pipe, normal)
{
	CPipeServer* _server;
	ASSERT_HRESULT_SUCCEEDED(CPipeServer::createInstance(&_server));
	std::unique_ptr<CPipeServer> server(_server);	CPipeClient* _client;
	ASSERT_HRESULT_SUCCEEDED(CPipeClient::createInstance(&_client));
	std::unique_ptr<CPipeClient> client;
	client.reset(_client);

	Sleep(5000);
	ASSERT_TRUE(server->isConnected());
	ASSERT_TRUE(client->isConnected());

	CPipe::Data dataToSend{ 1, 2, 3, 0, 5 };
	CPipe::Data receivedData;
	server->onReceived = [&](const CPipe::Data& data)
	{
		receivedData = data;
		return S_OK;
	};

	EXPECT_HRESULT_SUCCEEDED(client->send(dataToSend));
	Sleep(5000);

	EXPECT_EQ(dataToSend, receivedData);
}
