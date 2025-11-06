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
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[LobbyLanServer] Parse success: [%s]", LOBBY_SERVER_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyLanServer] Parse failed: [%s]", LOBBY_SERVER_CONFIG_FILE);
		jh_utility::CrashDump::Crash();
	}

	InitServerConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);
	
	if (false == InitSessionArray(maxSessionCnt))
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyLanServer] InitSessionArray failed.");	
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
	JobPtr jobPtr = MakeShared<jh_utility::Job>(g_memSystem, sessionId, type, packet); // MakeJob(sessionId, type, packet);

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