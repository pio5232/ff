#include "pch.h"
#include "LobbyDummyClient.h"
#include "DummyPacketBuilder.h"

jh_content::LobbyDummyClient::LobbyDummyClient() : IocpClient(L"LobbyDummy")
{
	jh_utility::Parser parser;

	parser.LoadFile(UP_DIR(LOBBY_DUMMY_FILE_NAME));
	parser.SetReadingCategory(LOBBY_DUMMY_CATEGORY_NAME);

	WCHAR ip[20];
	USHORT port;
	USHORT maxSessionCnt;
	DWORD concurrentWorkerThreadCount;

	USHORT lingerOnOff;
	USHORT lingerTime;
	ULONGLONG timeOut;

	bool succeeded = parser.GetValueWstr(L"serverIp", ip, ARRAY_SIZE(ip));
	succeeded &= parser.GetValue(L"serverPort", port);
	succeeded &= parser.GetValue(L"maxSessionCount", maxSessionCnt);
	succeeded &= parser.GetValue(L"concurrentWorkerThreadCount", concurrentWorkerThreadCount);

	succeeded &= parser.GetValue(L"lingerOnOff", lingerOnOff);
	succeeded &= parser.GetValue(L"lingerTime", lingerTime);
	succeeded &= parser.GetValue(L"TimeOut", timeOut);

	parser.CloseFile();

	if (true == succeeded)
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[LobbyDummyClient()] - 파일 파싱 성공 : %s", LOBBY_DUMMY_FILE_NAME);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyDummyClient()] - 파일 파싱 실패 : %s", LOBBY_DUMMY_FILE_NAME);
		jh_utility::CrashDump::Crash();
	}

	InitClientConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);

	if (false == InitSessionArray(maxSessionCnt))
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[GameLanClient()] - maxSession 초기화 실패");
		jh_utility::CrashDump::Crash();
	}
}

jh_content::LobbyDummyClient::~LobbyDummyClient()
{
}

void jh_content::LobbyDummyClient::OnRecv(ULONGLONG sessionId, PacketPtr dataBuffer, USHORT type)
{

}

void jh_content::LobbyDummyClient::OnConnected(ULONGLONG sessionId)
{
	PacketPtr hbPkt = jh_content::DummyPacketBuilder::BuildHeartbeatPacket();
	SendPacket(sessionId, hbPkt);

	PacketPtr logInReqPkt = jh_content::DummyPacketBuilder::BuildLoginRequestPacket();

	SendPacket(sessionId, logInReqPkt);

}

