#include "pch.h"
#include "LobbyDummyClient.h"
#include "DummyPacketBuilder.h"
#include "DummyUpdateSystem.h"
#include "Memory.h"
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

	m_pDummySystem = std::make_unique<jh_content::DummyUpdateSystem>(this);
	
}

jh_content::LobbyDummyClient::~LobbyDummyClient()
{
}

void jh_content::LobbyDummyClient::OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type)
{
	int threadNum = static_cast<int>(sessionId) % LOGIC_THREAD_COUNT;

	JobPtr job = MakeShared<jh_utility::Job>(g_memSystem, sessionId, type, packet); //MakeJob(sessionId, type, packet);

	m_pDummySystem->EnqueueJob(job, threadNum);
}

void jh_content::LobbyDummyClient::OnConnected(ULONGLONG sessionId)
{
	int threadNum = static_cast<int>(sessionId) % LOGIC_THREAD_COUNT;

	SessionConnectionEventPtr sessionConnEvent = MakeShared<jh_utility::SessionConnectionEvent>(g_memSystem, sessionId, jh_utility::SessionConnectionEventType::CONNECT);// MakeSystemJob(sessionId, jh_utility::SessionConnectionEventType::CONNECT);

	m_pDummySystem->EnqueueSessionConnEvent(sessionConnEvent, threadNum);
}

void jh_content::LobbyDummyClient::OnDisconnected(ULONGLONG sessionId)
{
	int threadNum = static_cast<int>(sessionId) % LOGIC_THREAD_COUNT;

	SessionConnectionEventPtr sessionConnEvent = MakeShared<jh_utility::SessionConnectionEvent>(g_memSystem, sessionId, jh_utility::SessionConnectionEventType::DISCONNECT);// MakeSystemJob(sessionId, jh_utility::SessionConnectionEventType::CONNECT);

	m_pDummySystem->EnqueueSessionConnEvent(sessionConnEvent, threadNum);
}

void jh_content::LobbyDummyClient::Monitor()
{
	wprintf(L"=================================================\n");

	wprintf(L" [Network] Sync + Async Send TPS : %ld\n", GetTotalSendCount());
	wprintf(L" [Network] Sync + Async Recv TPS : %ld\n", GetTotalRecvCount());

	wprintf(L" [Network] Async Send TPS : %ld\n", GetAsyncSendCount());
	wprintf(L" [Network] Async Recv TPS : %ld\n", GetAsyncRecvCount()); ;

	wprintf(L" [Network] Disconnected Session Count : %lld\n", GetDisconnectedCount());
	wprintf(L" [Network] Total Disconnected Session Count : %lld\n", GetTotalDisconnectedCount());
	wprintf(L"=================================================\n");

	wprintf(L" [Content] MAX Sessions : %d\n", GetMaxSessionCount());
	wprintf(L" [Content] Total Sessions : %d\n", GetSessionCount());
	wprintf(L" [Content] Total Dummies : %d\n", DummyData::aliveDummyCount);

	DummyUpdateSystem::EtcData etcData = m_pDummySystem->UpdateEtc();

	wprintf(L" [Content] Packet Re-Send Timeout Count [%d]s : %d\n", RE_SEND_TIMEOUT/1000, etcData.m_lReSendTimeoutCount);
	
	wprintf(L" [Content] Enter Failed.. DestroyedRoom_Count : [%d]\n",etcData.m_llDestroyedRoomErrorCount);
	wprintf(L" [Content] Enter Failed.. DiffRoom_Count : [%d]\n", etcData.m_llDiffRoomNameErrorCount);
	wprintf(L" [Content] Enter Failed.. FullRoom_Count : [%d]\n", etcData.m_llFullRoomErrorCount);
	wprintf(L" [Content] Enter Failed.. AlreadyRunning_Count : [%d]\n", etcData.m_llAlreadyRunningRoomErrorCount);



	wprintf(L"[RTT] : [%llu]ms \n",m_pDummySystem->GetRTT());
}

void jh_content::LobbyDummyClient::BeginAction()
{
	m_pDummySystem->Init();
}

void jh_content::LobbyDummyClient::EndAction()
{
	m_pDummySystem->Stop();
}

