#include "pch.h"
#include "LobbyDummyClient.h"
#include "LobbyDummyDefine.h"
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
	switch (type)
	{
	case jh_network::PacketType::ROOM_LIST_RESPONSE_PACKET: HandleRoomListResponsePacket(sessionId, dataBuffer); break;
	case jh_network::PacketType::LOG_IN_RESPONSE_PACKET: HandleLogInResponsePacket(sessionId, dataBuffer); break;
	case jh_network::PacketType::MAKE_ROOM_RESPONSE_PACKET: HandleMakeRoomResponsePacket(sessionId, dataBuffer); break;
	case jh_network::PacketType::ENTER_ROOM_RESPONSE_PACKET: HandleEnterRoomResponsePacket(sessionId, dataBuffer); break; 
	case jh_network::PacketType::CHAT_NOTIFY_PACKET: HandleChatNotifyPacket(sessionId, dataBuffer); break;
	case jh_network::PacketType::LEAVE_ROOM_RESPONSE_PACKET: HandleLeaveRoomResponsePacket(sessionId, dataBuffer); break;
		// EnterRoomNotify와 LeaveRoomNotify, GameReady는 더미에서 사용하지 않는다.

	}
}

void jh_content::LobbyDummyClient::OnConnected(ULONGLONG sessionId)
{
	PacketPtr hbPkt = jh_content::DummyPacketBuilder::BuildHeartbeatPacket();
	SendPacket(sessionId, hbPkt);

	PacketPtr logInReqPkt = jh_content::DummyPacketBuilder::BuildLoginRequestPacket();

	SendPacket(sessionId, logInReqPkt);

}

void jh_content::LobbyDummyClient::HandleRoomListResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::LobbyDummyClient::HandleLogInResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::LobbyDummyClient::HandleMakeRoomResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::LobbyDummyClient::HandleEnterRoomResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::LobbyDummyClient::HandleChatNotifyPacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::LobbyDummyClient::HandleLeaveRoomResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}
