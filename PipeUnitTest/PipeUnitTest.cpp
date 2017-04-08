// PipeUnitTest.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <log4cplus/configurator.h>

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tstring;

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
	int serverChannel;

	PipeTest() : serverChannel(-1) {}

	void SetUp() {
		server.reset(new CPipeServer());
		client.reset(new CPipeClient());
	}
	void TearDown() {
		EXPECT_HRESULT_SUCCEEDED(client->stop());
		EXPECT_HRESULT_SUCCEEDED(server->stop());
		EXPECT_FALSE(client->isConnected(0));
		EXPECT_FALSE(server->isConnected(serverChannel));
	}

	void connectAndWait(int serverChannelCount = 1);
	HRESULT bufferToString(CPipe::IBuffer* buffer, std::string& str);
};

void PipeTest::connectAndWait(int serverChannelCount /*= 1*/)
{
	CSafeEventHandle  hServerEvent(FALSE), hClientEvent(FALSE);
	HANDLE hEvents[] = { hServerEvent, hClientEvent };

	server->onConnected = [&](int ch)
	{
		serverChannel = ch;
		SetEvent(hServerEvent);
		return S_OK;
	};
	client->onConnected = [&](int)
	{
		SetEvent(hClientEvent);
		return S_OK;
	};

	ASSERT_HRESULT_SUCCEEDED(server->start(serverChannelCount));
	ASSERT_HRESULT_SUCCEEDED(client->connect(0));

	ASSERT_LT(WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, TRUE, 100), ARRAYSIZE(hEvents));

	EXPECT_TRUE(server->isConnected(serverChannel));
	EXPECT_TRUE(client->isConnected(0));
}

// Convert Received data in IBuffer object to string.
HRESULT PipeTest::bufferToString(CPipe::IBuffer * buffer, tstring & str)
{
	TCHAR* data;
	HR_ASSERT_OK(buffer->GetBuffer((void**)&data));
	DWORD size;
	HR_ASSERT_OK(buffer->GetSize(&size));

	str.assign(data, size / sizeof(tstring::value_type));

	return S_OK;
}

TEST_F(PipeTest, client_sends_data)
{
	connectAndWait();

	CSafeEventHandle  hServerEvent(FALSE), hClientEvent(FALSE);
	HANDLE hEvents[] = { hServerEvent, hClientEvent };

	data_t dataToSend{ 1, 2, 3, 0, 5 };
	data_t receivedData;
	server->onReceived = [&](int ch, CPipe::IBuffer* buffer)
	{
		BYTE* data;
		buffer->GetBuffer((void**)&data);
		DWORD size;
		buffer->GetSize(&size);
		receivedData = std::vector<BYTE>(data, data + size);
		SetEvent(hServerEvent);
		return S_OK;
	};

	CPipe::IBuffer* bufferSent = NULL;
	client->onCompletedToSend = [&](int, CPipe::IBuffer* buffer)
	{
		bufferSent = buffer;
		SetEvent(hClientEvent);
		return S_OK;
	};

	CComPtr<CPipe::IBuffer> buffer;
	ASSERT_HRESULT_SUCCEEDED(CPipe::IBuffer::createInstance(dataToSend.size(), dataToSend.data(), &buffer));
	ASSERT_HRESULT_SUCCEEDED(client->send(0, buffer));
	ASSERT_LT(WaitForMultipleObjects(2, hEvents, TRUE, 1000), ARRAYSIZE(hEvents));

	EXPECT_EQ(dataToSend, receivedData);
	EXPECT_EQ(buffer, bufferSent);
}

TEST_F(PipeTest, server_sends_multiple_data)
{
	connectAndWait();

	std::vector<tstring> datasToSend{
		_T("wstring: A type that describes a secialization of the temlate class basic_staring with elements of type wchar_t."),
		_T("Writing a Mutithireaded Win32 Program"),
		_T("Sharing Common Resources Between Threads."),
		_T("mutable std::mutex m;"),
		_T("std::lock_guard<std::mutex> g(m); // lock mutex"),
		_T("項目16: const メンバ関数はスレッドセーフにする"),
	};

	// Received data
	std::vector<tstring> datasReceived;

	client->onReceived = [&](int ch, CPipe::IBuffer* buffer)
	{
		tstring str;
		HR_ASSERT_OK(bufferToString(buffer, str));
		LOG4CPLUS_INFO(logger, "Received: '" << str.c_str() << "'");
		datasReceived.push_back(str);
		return S_OK;
	};

	// Event to signal that all data have been sent.
	CSafeEventHandle  hSentEvent(FALSE);

	server->onCompletedToSend = [&](int ch, CPipe::IBuffer* buffer)
	{
		tstring str;
		HR_ASSERT_OK(bufferToString(buffer, str));
		LOG4CPLUS_INFO(logger, "Sent    : '" << str.c_str() << "'");
		if (str == *datasToSend.rbegin()) {
			WIN32_ASSERT(SetEvent(hSentEvent));
		}
		return S_OK;
	};

	std::for_each(datasToSend.begin(), datasToSend.end(), [&](const tstring& str)
	{
		// Send string including terminating zero.
		CComPtr<CPipe::IBuffer> buffer;
		ASSERT_HRESULT_SUCCEEDED(CPipe::IBuffer::createInstance(str.size() * sizeof(tstring::value_type), str.c_str(), &buffer));
		ASSERT_HRESULT_SUCCEEDED(server->send(0, buffer));
	});

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(hSentEvent, 1000));
	Sleep(100);

	ASSERT_EQ(datasToSend.size(), datasReceived.size());

	// All datas should be received once.
	for(size_t i = 0; i < datasToSend.size(); i++) {
		EXPECT_EQ(datasToSend[i], datasReceived[i]);
	}
}
