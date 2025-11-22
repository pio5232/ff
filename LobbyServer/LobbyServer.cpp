#include "pch.h"
#include "LobbyServer.h"
#include "LanServer.h"
#include "LobbySystem.h"
#include "Job.h"
#include "Memory.h"

jh_content::LobbyServer::LobbyServer() : IocpServer{ LOBBY_SERVER_SAVE_FILE_NAME }/*, _canCheckHeartbeat(true)*/
{
	jh_utility::Parser parser;

	parser.LoadFile(UP_DIR(LOBBY_SERVER_CONFIG_FILE));
	parser.SetReadingCategory(LOBBY_CATEGORY_NAME);

	WCHAR ip[20];
	USHORT port = 0;
	USHORT maxSessionCnt = 0;
	DWORD concurrentWorkerThreadCount = 0;

	USHORT lingerOnOff = 0;
	USHORT lingerTime = 0;
	ULONGLONG timeOut = 0;

	bool succeeded = parser.GetValueWstr(L"serverIp", ip, ARRAY_SIZE(ip));
	succeeded &= parser.GetValue(L"serverPort", port);
	succeeded &= parser.GetValue(L"maxSessionCount", maxSessionCnt);
	succeeded &= parser.GetValue(L"concurrentWorkerThreadCount", concurrentWorkerThreadCount);

	succeeded &= parser.GetValue(L"lingerOnOff", lingerOnOff);
	succeeded &= parser.GetValue(L"lingerTime", lingerTime);
	succeeded &= parser.GetValue(L"TimeOut", timeOut);

	parser.SetReadingCategory(LOBBY_DATA_CATEGORY_NAME);

	USHORT maxRoomCnt = 0;
	USHORT maxRoomUserCnt = 0;

	succeeded &= parser.GetValue(L"maxRoomCount", maxRoomCnt);
	succeeded &= parser.GetValue(L"maxRoomUserCount", maxRoomUserCnt);

	parser.CloseFile();

	if (true == succeeded)
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[LobbyServer] Parsing LobbyServer complete. File: [%s]", LOBBY_SERVER_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyServer] Parsing LobbyServer failed.");
		jh_utility::CrashDump::Crash();
	}

	InitServerConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);
	
	if (false == InitSessionArray(maxSessionCnt))
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyServer] InitSessionArray failed.");		
		jh_utility::CrashDump::Crash();
	}

	m_pLanServer = std::make_unique<jh_content::LobbyLanServer>();

	m_pLobbySystem = std::make_unique<jh_content::LobbySystem>(this, maxRoomCnt, maxRoomUserCnt);

	m_pLobbySystem->Init();
	
	m_pLanServer->SetLobbySystem(m_pLobbySystem.get());
	m_pLanServer->Init();
	
}

jh_content::LobbyServer::~LobbyServer()
{

}

void jh_content::LobbyServer::BeginAction()
{
	Listen();

	m_pLanServer->Start();

	m_pLanServer->Listen();
}

void jh_content::LobbyServer::EndAction()
{
	m_pLobbySystem->Stop();

	m_pLanServer->Stop();
	
}
void jh_content::LobbyServer::OnRecv(ULONGLONG sessionId, PacketBufferRef packet, USHORT type)
{
	JobRef job = jh_memory::MakeShared<jh_utility::Job>(sessionId, type, packet); //MakeJob(sessionId, type, packet);

	m_pLobbySystem->EnqueueJob(job);
}
void jh_content::LobbyServer::OnConnected(ULONGLONG sessionId)
{
	SessionConnectionEventRef sessionConnEvent = jh_memory::MakeShared<jh_utility::SessionConnectionEvent>(sessionId, jh_utility::SessionConnectionEventType::CONNECT);// MakeSystemJob(sessionId, jh_utility::SessionConnectionEventType::CONNECT);

	m_pLobbySystem->EnqueueSessionConnEvent(sessionConnEvent);
}

void jh_content::LobbyServer::OnDisconnected(ULONGLONG sessionId)
{
	SessionConnectionEventRef sessionConnEvent = jh_memory::MakeShared<jh_utility::SessionConnectionEvent>(sessionId, jh_utility::SessionConnectionEventType::DISCONNECT); // MakeSystemJob(sessionId, jh_utility::SessionConnectionEventType::DISCONNECT);

	m_pLobbySystem->EnqueueSessionConnEvent(sessionConnEvent);
}

void jh_content::LobbyServer::Monitor()
{
	wprintf(L" [Network] Sync + Async Send TPS : %ld\n", GetTotalSendCount());
	wprintf(L" [Network] Sync + Async Recv TPS : %ld\n", GetTotalRecvCount());

	wprintf(L" [Network] Async Send TPS : %ld\n", GetAsyncSendCount());
	wprintf(L" [Network] Async Recv TPS : %ld\n", GetAsyncRecvCount());	;

	wprintf(L" [Network] Disconnected Session Count : %lld\n", GetDisconnectedCount());
	wprintf(L" [Network] Total Disconnected Session Count : %lld\n", GetTotalDisconnectedCount());

	wprintf(L" [Lobby Server] Accepted Count: %lld\n", GetTotalAcceptedCount());
	wprintf(L" [Lobby Server] Sessions Count : %ld\n", GetSessionCount());
	wprintf(L" [LAN Server]   Sessions Count : %ld\n", m_pLanServer->GetSessionCount());
}

void jh_content::LobbyServer::GetInvalidMsgCnt()
{
	m_pLobbySystem->GetInvalidMsgCnt();
}
