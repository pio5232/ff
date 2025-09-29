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
	// 파일 로드
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
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"[GameServer] [FileName : %s] 파싱 완료", GAME_SERVER_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[GameSever] 파싱 실패");
		jh_utility::CrashDump::Crash();
	}

	// 기본 설정 셋팅
	InitServerConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);
	//if (false == InitSessionArray(maxSessionCnt))
	//{
	//	_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[LobbyServer()] - maxSession 초기화 실패");
	//	jh_utility::CrashDump::Crash();
	//}
	//
	// GameSystem 생성
	m_pGameSystem = std::make_unique<jh_content::GameSystem>(this);

	// LanClient 생성
	m_pGameLanClient = std::make_unique<jh_content::GameLanClient>();

	m_pGameLanClient->SetGameSystem(m_pGameSystem.get());

}

//jh_network::GameServer::GameServer(const NetAddress& netAddr, uint maxSessionCnt, jh_network::SessionCreator creator) : ServerBase(netAddr, maxSessionCnt, creator), _gameInfo{}, _isRunning(false), _loadCompletedCnt(0)
//{
//	_gameWorld = std::make_unique<jh_content::GameWorld>();
//
//	_monitor = std::make_unique<jh_utility::GameMonitor>(_sessionMgr.get(), _gameWorld->GetSectorManagerConst());
//
//	std::random_device rd;
//	std::mt19937_64 generator(rd()); // 시드 섞음.
//	_gameInfo.m_ullEnterToken = generator();
//
//	jh_network::GameClientPacketHandler::Init();
//
//	printf("----------------------------------------------------- GAME SERVER -------\n");
//}

jh_content::GameServer::~GameServer()
{
}

//void jh_content::GameServer::Init(uint16 roomNumber, uint16 requiredUsers, uint16 m_usMaxUserCnt)
//{
//	_sessionMgr->SetMaxCount(m_usMaxUserCnt);
//	_gameWorld->SetDSCount(m_usMaxUserCnt);
//	jh_content::UserManager::GetInstance().ReserveUMapSize(requiredUsers, m_usMaxUserCnt);
//
//	_gameInfo.m_usRoomNumber = roomNumber;
//	_gameInfo.m_usRequiredUserCnt = requiredUsers;
//	_gameInfo.m_usMaxUserCnt = m_usMaxUserCnt;
//
//	_isRunning.store(false);
//}

bool jh_content::GameServer::OnConnectionRequest(const SOCKADDR_IN& clientInfo)
{
	return true;
}



void jh_content::GameServer::OnError(int errCode, WCHAR* cause)
{
}

void jh_content::GameServer::OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type)
{
	JobPtr job = MakeShared<jh_utility::Job>(g_memAllocator, sessionId, type, packet);

	m_pGameSystem->EnqueueJob(job);
}

void jh_content::GameServer::OnConnected(ULONGLONG sessionId)
{
	SessionConnectionEventPtr sessionConnectionEvent = MakeShared<jh_utility::SessionConnectionEvent>(g_memAllocator, sessionId, jh_utility::SessionConnectionEventType::CONNECT);

	m_pGameSystem->EnqueueSessionConnEvent(sessionConnectionEvent);
}

void jh_content::GameServer::OnDisconnected(ULONGLONG sessionId)
{
	SessionConnectionEventPtr sessionConnectionEvent = MakeShared<jh_utility::SessionConnectionEvent>(g_memAllocator, sessionId, jh_utility::SessionConnectionEventType::DISCONNECT);

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


//void jh_content::GameServer::LanClientConnect(const NetAddress& netAddr)
//{
//	// 포트 번호 동적으로 할당 시 진짜 포트를 찾는다.
//	SOCKADDR_IN realAddr;
//	int addrLen = sizeof(realAddr);cc
//
//	if (getsockname(GetListenSock(), (SOCKADDR*)&realAddr, &addrLen) == 0)
//	{
//		_gameInfo.m_usPort = ntohs(realAddr.sin_port);
//	}
//	else
//	{
//		printf("Find Real Port Failed - %d\n", GetLastError());
//	}
//
//	_lanClient = std::make_shared<jh_network::LanClient>(netAddr, [self = shared_from_this()]() { return std::static_pointer_cast<jh_network::Session>(std::make_shared<jh_network::LanClientSession>(self)); });
//
//	_lanClient->Connect();
//}
//
//void jh_content::GameServer::LanClientDisconnect()
//{
//	_lanClient->Disconnect();
//}
//
//ErrorCode jh_content::GameServer::TryRun()
//{
//	if (_isRunning.load() == true)
//	{
//		printf("GameServer 이미 실행중!!\n");
//
//		return ErrorCode::GAME_SERVER_IS_RUNNING;
//	}
//	if (_gameInfo.m_usRequiredUserCnt.load() == jh_content::UserManager::GetInstance().GetPlayerCount())
//	{
//		bool expected = false;
//		bool desired = true;
//
//		if (_isRunning.compare_exchange_strong(expected, desired) == true)
//		{
//			_gameWorld->Init(_gameInfo.m_usMaxUserCnt, _gameInfo.m_usRequiredUserCnt);
//
//			printf("Start Game\n");
//		}
//	}
//
//	return ErrorCode::NONE;
//}
//
//void jh_content::GameServer::CheckLoadingAndStartLogic()
//{
//	int prevCnt = _loadCompletedCnt.fetch_add(1);
//	if (prevCnt == (_gameInfo.m_usRequiredUserCnt.load() - 1))
//	{
//		printf("Check Loading & Start Logic!!! [player+ai count : %d]\n", _gameInfo.m_usMaxUserCnt);
//		jh_network::GameStartNotifyPacket gameStartNotifyPacket;
//
//		// 이거 추후에 패킷 구조 바뀌게되면 (Header말고 데이터 있으면 따로 <<를 통해서 넣어주는 구조로 다시 만들어야함.) AAA
//		SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakePacket(gameStartNotifyPacket);
//
//		jh_content::UserManager::GetInstance().SendToAllPlayer(sendBuffer);
//
//		_gameWorld->Start();
//		// Start Logic
//		// StartPacket. timestamp 포함해서 보내기
//	}
//}
//
//GamePlayerPtr jh_content::GameServer::CreateUser(GameSessionPtr gameSessionPtr)
//{
//	GamePlayerPtr playerPtr = jh_content::UserManager::GetInstance().CreateUser(gameSessionPtr, _gameWorld.get());
//
//	WorldChatPtr worldChatPtr = _gameWorld->GetWorldChat();
//
//	GameSessionPtr thisSessionPtr = static_pointer_cast<GameSession>(gameSessionPtr);
//
//	worldChatPtr->DoAsync(&jh_content::WorldChat::RegisterMember, thisSessionPtr);
//
//	EnqueueAction([this, playerPtr]() {_gameWorld->AddEntity(playerPtr); }, true);
//
//	return playerPtr;
//}
//
//ErrorCode jh_content::GameServer::RemoveUser(GamePlayerPtr gamePlayerPtr)
//{
//	ULONGLONG userId = gamePlayerPtr->GetUserId();
//
//	ErrorCode ret = jh_content::UserManager::GetInstance().RemoveUser(userId);
//
//	WorldChatPtr worldChatPtr = _gameWorld->GetWorldChat();
//
//	worldChatPtr->DoAsync(&jh_content::WorldChat::RemoveMember, userId);
//
//	EnqueueAction([this, gamePlayerPtr]() {
//
//		if (false == gamePlayerPtr->IsDead())
//		{
//			_gameWorld->RemoveEntity(gamePlayerPtr->GetEntityId());
//
//		}
//		//_gameWorld->InvalidateWinner(gamePlayerPtr->GetUserId());
//
//		}, true);
//
//	return ret;
//}
//
//void jh_content::GameServer::EnqueueAction(Action&& action, bool mustEnqueue)
//{
//	_gameWorld->TryEnqueueAction(std::move(action), mustEnqueue);
//}
//
//void jh_content::GameServer::EnqueueTimerAction(TimerAction&& timerAction)
//{
//	_gameWorld->TryEnqueueTimerAction(std::move(timerAction));
//}
//
//WorldChatPtr jh_content::GameServer::GetWorldChatPtr()
//{
//	return _gameWorld->GetWorldChat();
//}
//
//void jh_content::GameServer::ProxyAttackPacket(GameSessionPtr gameSession, AttackRequestPacket packet)
//{
//	GamePlayerPtr gamePlayerPtr = gameSession->GetPlayer();
//	_gameWorld->TryEnqueueAction([this, gamePlayerPtr, packet]() {
//
//		if (gamePlayerPtr->IsDead())
//			return;
//
//		_gameWorld->ProcessAttack(gamePlayerPtr, packet);
//
//		});
//}
//
//void jh_content::GameServer::CheckHeartbeat()
//{
//	printf("[ Check Heartbeat Start ]\n");
//	while (_canCheckHeartbeat)
//	{
//		ULONGLONG now = jh_utility::GetTimeStamp();
//
//		_sessionMgr->CheckHeartbeatTimeOut(now);
//
//		Sleep(3000);
//	}
//	printf("[ Check Heartbeat End ]\n");
//}
