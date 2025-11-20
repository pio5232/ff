#include "pch.h"
#include <chrono>
#include <random>
#include <WS2tcpip.h>
#include "GameServer.h"
#include "Memory.h"
#include "Job.h"
#include "GameSystem.h"
#include "GameLanClient.h"

jh_content::GameServer::GameServer() : jh_network::IocpServer(GAME_SERVER_SAVE_FILE_NAME)
{
	jh_utility::Parser parser;

	parser.LoadFile(UP_DIR(GAME_SERVER_CONFIG_FILE));
	parser.SetReadingCategory(GAME_CATEGORY_NAME);

	WCHAR ip[IP_STRING_LEN];
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

	parser.CloseFile();

	if (true == succeeded)
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[GameServer] Parse complete. File : [%s]", GAME_SERVER_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[GameServer] Parse failed.");
		jh_utility::CrashDump::Crash();
	}

	// 기본 설정 셋팅
	InitServerConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);

	m_pGameSystem = std::make_unique<jh_content::GameSystem>(this);

	// LanClient 생성
	m_pGameLanClient = std::make_unique<jh_content::GameLanClient>();

	m_pGameLanClient->SetGameSystem(m_pGameSystem.get());

}

jh_content::GameServer::~GameServer()
{
}

bool jh_content::GameServer::OnConnectionRequest(const SOCKADDR_IN& clientInfo)
{
	return true;
}



void jh_content::GameServer::OnError(int errCode, WCHAR* cause)
{
}

void jh_content::GameServer::OnRecv(ULONGLONG sessionId, PacketBufferRef packet, USHORT type)
{
	JobRef job = jh_memory::MakeShared<jh_utility::Job>(sessionId, type, packet);

	m_pGameSystem->EnqueueJob(job);
}

void jh_content::GameServer::OnConnected(ULONGLONG sessionId)
{
	SessionConnectionEventRef sessionConnectionEvent = jh_memory::MakeShared<jh_utility::SessionConnectionEvent>(sessionId, jh_utility::SessionConnectionEventType::CONNECT);

	m_pGameSystem->EnqueueSessionConnEvent(sessionConnectionEvent);
}

void jh_content::GameServer::OnDisconnected(ULONGLONG sessionId)
{
	SessionConnectionEventRef sessionConnectionEvent = jh_memory::MakeShared<jh_utility::SessionConnectionEvent>(sessionId, jh_utility::SessionConnectionEventType::DISCONNECT);

	m_pGameSystem->EnqueueSessionConnEvent(sessionConnectionEvent);
}

void jh_content::GameServer::BeginAction()
{
	Listen();

	m_pGameSystem->Init();

	m_pGameLanClient->Start();
}

void jh_content::GameServer::EndAction()
{
	m_pGameSystem->Stop();

	m_pGameLanClient->Stop();
}

void jh_content::GameServer::Monitor()
{
	wprintf(L" [Game Server] Sessions : %d\n", GetSessionCount());
}