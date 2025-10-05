#include "pch.h"
#include "LanServer.h"
#include "LobbyLanSystem.h"
#include "LobbySystem.h"
#include "Memory.h"

jh_content::LobbyLanServer::LobbyLanServer() : IocpServer(LAN_SAVE_FILE_NAME), m_pLobbyLanSystem(nullptr)
{
	jh_utility::Parser parser;

	parser.LoadFile(UP_DIR(LAN_SERVER_CONFIG_FILE));
	parser.SetReadingCategory(LAN_CATEGORY_NAME);

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
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[LobbyLanServer()] - 파일 파싱 성공 : %s", LOBBY_SERVER_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyLanServer()] - 파일 파싱 실패 : %s",LOBBY_SERVER_CONFIG_FILE);
		jh_utility::CrashDump::Crash();
	}

	InitServerConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);
	
	if (false == InitSessionArray(maxSessionCnt))
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyLanServer()] - maxSession 초기화 실패");
		jh_utility::CrashDump::Crash();
	}

	m_pLobbyLanSystem = std::make_unique<jh_content::LobbyLanSystem>(this);
}

jh_content::LobbyLanServer::~LobbyLanServer()
{

}

bool jh_content::LobbyLanServer::OnConnectionRequest(const SOCKADDR_IN& clientInfo)
{
	return true;
}

void jh_content::LobbyLanServer::OnError(int errCode, WCHAR* cause)
{
}


void jh_content::LobbyLanServer::OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type)
{
	JobPtr jobPtr = MakeShared<jh_utility::Job>(g_memAllocator, sessionId, type, packet); // MakeJob(sessionId, type, packet);

	m_pLobbyLanSystem->EnqueueJob(jobPtr);

}
void jh_content::LobbyLanServer::OnConnected(ULONGLONG sessionId)
{

}
void jh_content::LobbyLanServer::OnDisconnected(ULONGLONG sessionId)
{

}

void jh_content::LobbyLanServer::Init()
{
	m_pLobbyLanSystem->Init();
}

void jh_content::LobbyLanServer::EndAction()
{
	m_pLobbyLanSystem->Stop();
}

void jh_content::LobbyLanServer::SetLobbySystem(jh_content::LobbySystem* lobbySystem)
{
	m_pLobbyLanSystem->SetLobbySystem(lobbySystem);
}

//void jh_network::LobbyLanServer::InitPacketFuncs()
//{
//	m_packetFuncDic.clear();
//
//	m_packetFuncDic[GAME_SERVER_LAN_INFO_PACKET] = LobbyLanServer::HandleLanInfoNotifyPacket; // ip Port
//	m_packetFuncDic[GAME_SERVER_SETTING_REQUEST_PACKET] = LobbyLanServer::HandleGameSettingRequestPacket; // completePacket
//
//}
//
//void jh_network::LobbyLanServer::HandleLanInfoNotifyPacket(ULONGLONG sessionId, PacketPtr& packet)
//{
//	printf("Lan Info Notify Packet Recv\n");
//	jh_network::GameServerLanInfoPacket lanInfoPacket;
//	
//	packet->GetData(reinterpret_cast<char*>(&lanInfoPacket.ipStr[0]), sizeof(lanInfoPacket.ipStr));
//
//	*packet >> lanInfoPacket.port >> lanInfoPacket.roomNum >> lanInfoPacket.xorToken;
//
//	wprintf(L"size : [ %d ]\n", lanInfoPacket.size);
//	wprintf(L"type : [ %d ]\n", lanInfoPacket.type);
//	wprintf(L"ip: [ %s ]\n", lanInfoPacket.ipStr);
//	wprintf(L"port : [ %d ]\n", lanInfoPacket.port);
//	wprintf(L"Room : [ %d ]\n", lanInfoPacket.roomNum);
//	wprintf(L"xorToken : [ %llu ], After : [%llu]\n", lanInfoPacket.xorToken, lanInfoPacket.xorToken ^ xorTokenKey);
//
//	PacketPtr sendBuffer = jh_content::PacketBuilder::// C_Network::BufferMaker::MakeSendBuffer(sizeof(lanInfoPacket));
//
//	*sendBuffer << lanInfoPacket.size << lanInfoPacket.type;
//
//	sendBuffer->PutData(reinterpret_cast<const char*>(lanInfoPacket.ipStr), IP_STRING_LEN * sizeof(WCHAR));
//
//	*sendBuffer << lanInfoPacket.port << lanInfoPacket.roomNum << lanInfoPacket.xorToken;
//
//	RoomPtr roomPtr = C_Network::RoomManager::GetInstance().GetRoom(lanInfoPacket.roomNum);
//
//	if (roomPtr == nullptr)
//	{
//		// LOG
//		return ErrorCode::ACCESS_DESTROYED_ROOM;
//	}
//
//	roomPtr->SendToAll(sendBuffer);
//
//	return ErrorCode::NONE;
//}
//
//void jh_network::LobbyLanServer::HandleGameSettingRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
//{
//	printf("Game Setting Request Packet Recv\n");
//	uint16 roomNum = C_Network::RoomManager::GetInstance().PopRoomNum();
//	
//	RoomPtr roomPtr = C_Network::RoomManager::GetInstance().GetRoom(roomNum);
//
//	if (roomPtr == nullptr)
//	{
//		// LOG
//		printf("GameSetting.. Room is Invalid\n");
//		return ErrorCode::ACCESS_DESTROYED_ROOM;
//	}
//
//	C_Network::GameServerSettingResponsePacket settingResponsePacket;
//
//	settingResponsePacket.roomNum = roomNum;
//	settingResponsePacket.requiredUsers = roomPtr->GetCurUserCnt();
//	settingResponsePacket.m_usMaxUserCnt = roomPtr->GetMaxUserCnt();
//	
//	SharedSendBuffer sendBuffer = C_Network::BufferMaker::MakeSendBuffer(sizeof(settingResponsePacket));
//
//	*sendBuffer << settingResponsePacket.size << settingResponsePacket.type << settingResponsePacket.roomNum << settingResponsePacket.requiredUsers << settingResponsePacket.m_usMaxUserCnt;
//	
//	lanSessionPtr->Send(sendBuffer);
//
//	return ErrorCode::NONE;
//}
// 