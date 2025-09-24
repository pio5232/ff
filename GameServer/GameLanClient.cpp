#include "pch.h"
#include "GameLanClient.h"
#include "GameServer.h"
#include "Memory.h"
#include "GameSystem.h"


jh_content::GameLanClient::GameLanClient() : jh_network::IocpClient(GAME_LAN_CLIENT_SAVE_FILE_NAME)
{
	jh_utility::Parser parser;

	parser.LoadFile(GAME_LAN_CLIENT_CONFIG_FILE);
	parser.SetReadingCategory(GAME_LAN_CATEGORY_NAME);

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
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[GameLanClient()] - 파일 파싱 성공 : %s", GAME_LAN_CLIENT_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[GameLanClient()] - 파일 파싱 실패 : %s", GAME_LAN_CLIENT_CONFIG_FILE);
		jh_utility::CrashDump::Crash();
	}

	InitClientConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);

	if (false == InitSessionArray(maxSessionCnt))
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyLanServer()] - maxSession 초기화 실패");
		jh_utility::CrashDump::Crash();
	}
}

jh_content::GameLanClient::~GameLanClient()
{
}

void jh_content::GameLanClient::OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type)
{
	GameLanRequestPtr lanRequest = MakeShared<GameLanRequest>(g_memAllocator, sessionId, type, packet); // MakeJob(sessionId, type, packet);

	m_pGameSystem->EnqueueLanRequest(lanRequest);
}

void jh_content::GameLanClient::OnConnected(ULONGLONG sessionId)
{
}
