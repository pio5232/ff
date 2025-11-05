#include "pch.h"
#include "GameSystem.h"
#include "UserManager.h"
#include <random>
#include "GameWorld.h"
#include "PacketBuilder.h"
#include "GamePlayer.h"
#include "Memory.h"
#include "User.h"
#include "GameLanClient.h"

void jh_content::GameSystem::Init()
{
	m_packetFuncDic[jh_network::ENTER_GAME_REQUEST_PACKET] = &GameSystem::HandleEnterGameRequestPacket;
	m_packetFuncDic[jh_network::GAME_LOAD_COMPELTE_PACKET] = &GameSystem::HandleLoadCompletedPacket;
	m_packetFuncDic[jh_network::MOVE_START_REQUEST_PACKET] = &GameSystem::HandleMoveStartRequestPacket;
	m_packetFuncDic[jh_network::MOVE_STOP_REQUEST_PACKET] = &GameSystem::HandleMoveStopRequestPacket;
	m_packetFuncDic[jh_network::ATTACK_REQUEST_PACKET] = &GameSystem::HandleAttackRequestPacket;
	m_packetFuncDic[jh_network::CHAT_TO_ROOM_REQUEST_PACKET] = &GameSystem::HandleChatRequestPacket;
	
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, GameSystem::LogicThreadFunc, (LPVOID)this, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameSystem Init] - m_hLogicThread 생성 실패");
		jh_utility::CrashDump::Crash();
	}
}

void jh_content::GameSystem::Stop()
{
	m_bIsRunning.store(false);

	if (nullptr != m_hLogicThread)
	{
		DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

		if (ret != WAIT_OBJECT_0)
		{
			DWORD getLastError = GetLastError();

			_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameSystem - Stop] 로직 스레드 종료 오류 GetLastError : [%u]", getLastError);

			jh_utility::CrashDump::Crash();
		}

		CloseHandle(m_hLogicThread);

		m_hLogicThread = nullptr;

		m_netJobQueue.Clear();
		m_gameLanRequestQueue.Clear();
		m_sessionConnEventQueue.Clear();

	}

	m_pGameWorld->Stop();
}

jh_content::GameSystem::GameSystem(jh_network::IocpServer* owner) : m_hLogicThread(nullptr), m_bIsRunning(false), m_dwLoadCompletedCnt(0), m_pOwner(owner), m_gameInfo{}
{
	auto sendPacketFunc = [this](ULONGLONG sessionId, PacketPtr& _packet)
		{
			m_pOwner->SendPacket(sessionId, _packet);
		};

	m_pUserManager = std::make_unique<jh_content::UserManager>(sendPacketFunc);
	m_pGameWorld = std::make_unique<jh_content::GameWorld>(m_pUserManager.get(), sendPacketFunc);

	std::random_device rd; // 난수 생성기
	std::mt19937_64 generator(rd()); // 시드 섞음.
	m_gameInfo.m_ullEnterToken = generator();

}

jh_content::GameSystem::~GameSystem()
{
	if (false != m_bIsRunning)
	{
		Stop();
	}
}

void jh_content::GameSystem::SetGameInfo(USHORT roomNumber, USHORT requiredUsers, USHORT maxUsers)
{
	m_gameInfo.m_usRoomNumber = roomNumber;
	m_gameInfo.m_usMaxUserCnt = maxUsers;
	m_gameInfo.m_usRequiredUserCnt = requiredUsers;

	m_pUserManager->ReserveUMapSize(requiredUsers, maxUsers);
}


unsigned jh_content::GameSystem::LogicThreadFunc(LPVOID lparam)
{
	jh_content::GameSystem* gameInstance = static_cast<GameSystem*>(lparam);

	if (nullptr == gameInstance)
		return 0;

	gameInstance->m_bIsRunning = true;
	gameInstance->GameLogic();

	return 0;
}

void jh_content::GameSystem::GameLogic()
{
	jh_utility::Timer timer;

	timer.Start();

	float deltaSum = 0;
	// 조건 바꾸기.
	while (true == m_bIsRunning.load())
	{
		float deltaTime = timer.Lap<float>();

		if (deltaTime > limitDeltaTime)
			deltaTime = limitDeltaTime;


		// 패킷 처리.
		ProcessNetJob();

		// 연결 처리.
		ProcessSessionConnectionEvent();

		// Lan 요청 처리.
		ProcessLanRequest();

		// 타이머 작업 처리.
		m_pGameWorld->ProcessTimerActions();

		deltaSum += deltaTime;

		m_pGameWorld->Update(deltaTime);

		Sleep(0);
	}

	printf("Update Thread Exit...\n");
}

void jh_content::GameSystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return;

	(this->*m_packetFuncDic[packetType])(sessionId, packet);
	
	return;
}

void jh_content::GameSystem::ProcessNetJob()
{
	static thread_local alignas(64) std::queue<JobPtr> gameJobQ;
	std::queue<JobPtr>	emptyQ;

	m_netJobQueue.Swap(gameJobQ);

	while(gameJobQ.size() > 0)
	{
		JobPtr& job = gameJobQ.front();

		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
		
		gameJobQ.pop();
	}

	gameJobQ.swap(emptyQ);
}

void jh_content::GameSystem::ProcessSessionConnectionEvent()
{
	static alignas(64) std::queue<SessionConnectionEventPtr> sessionConnEventQ;
	std::queue<SessionConnectionEventPtr> emptyQ;

	m_sessionConnEventQueue.Swap(sessionConnEventQ);

	while(sessionConnEventQ.size() > 0)
	{
		SessionConnectionEventPtr& sessionConnEvent = sessionConnEventQ.front();

		ULONGLONG sessionId = sessionConnEvent->m_ullSessionId;
		switch (sessionConnEvent->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{
			// m_pUserManager->CreateGuest(sessionId);
			//m_pUserManager->CreateUser(job->m_llSessionId);
			// 유저 접속 처리.
		}
		break;
		case jh_utility::SessionConnectionEventType::DISCONNECT:
		{
			UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

			if (nullptr == userPtr)
			{
				_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[ProcessSessionConnectionEvent] 유저가 존재하지 않습니다. SessionId: [%llu]", sessionId);
				break;
			}
			ULONGLONG userId = userPtr->GetUserId();

			GamePlayerPtr gamePlayerPtr = userPtr->GetPlayer();

			if (nullptr != gamePlayerPtr)
			{
				ULONGLONG entityId = gamePlayerPtr->GetEntityId();
				m_pUserManager->DeleteEntityIdToUser(entityId);
				m_pGameWorld->RemoveEntity(entityId);
			}
			m_pUserManager->RemoveUser(sessionId);
			// 유저 접속 종료 처리.
		}
		break;
		default:break;
		}

		sessionConnEventQ.pop();
	}

	sessionConnEventQ.swap(emptyQ);
}

void jh_content::GameSystem::ProcessLanRequest()
{
	static thread_local alignas(64) std::queue<GameLanRequestPtr> gameLanRequestJobQ;
	std::queue<GameLanRequestPtr> emptyQ;

	m_gameLanRequestQueue.Swap(gameLanRequestJobQ);

	while(gameLanRequestJobQ.size() > 0)
	{
		GameLanRequestPtr& req = gameLanRequestJobQ.front();

		if (req->m_ullSessionId == INVALID_SESSION_ID)
			continue;

		switch (req->m_usMsgType)
		{
		case jh_network::GAME_SERVER_SETTING_RESPONSE_PACKET:HandleGameServerSettingResponsePacket(req->m_ullSessionId, req->m_pPacket, req->m_pClient); break;
			// 추가

		default:break;
		}

		gameLanRequestJobQ.pop();
	}

	gameLanRequestJobQ.swap(emptyQ);
}

void jh_content::GameSystem::HandleEnterGameRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	//jh_network::EnterGameRequestPacket packet;
	ULONGLONG userId;
	ULONGLONG token;
	*packet >> userId >> token;

	printf("Enter Game Packet Recv.. %llu, userId : %llu\n", sessionId, userId);

	if (m_gameInfo.m_ullEnterToken != token)
	{
		printf("WRONG TOKEN!!\n");
		PacketPtr errPkt = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::CONNECTED_FAILED_WRONG_TOKEN);

		m_pOwner->SendPacket(sessionId, errPkt);

		return;
	}

	// TODO : 0916 컨텐츠에서 접속의 관리를 하는 클래스가 sessionid, userid, player를 관리해야하는 방법에 대해서 생각
	UserPtr newUser = m_pUserManager->CreateUser(sessionId, userId);
	
	if (nullptr == newUser)
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[HandleEnterGameRequestPacket] - 세션 또는 유저의 중복으로 인해 유저가 생성되지 않았습니다.", userId);
		return;
	}
	PacketPtr enterGameResponsePkt = jh_content::PacketBuilder::BuildEnterGameResponsePacket();
	m_pOwner->SendPacket(sessionId, enterGameResponsePkt);

	GamePlayerPtr newPlayer = m_pGameWorld->CreateGamePlayer(newUser);

	newUser->SetPlayer(newPlayer);

	m_pGameWorld->AddEntity(static_pointer_cast<jh_content::Entity>(newPlayer));
	m_pUserManager->RegisterEntityIdToUser(newPlayer->GetEntityId(), newUser);
	
	jh_network::MakeMyCharacterPacket makeMyCharacterPacket;
	
	ULONGLONG entityId = newPlayer->GetEntityId();
	const Vector3& pos = newPlayer->GetPosition();

	PacketPtr makeMyCharacterPkt = jh_content::PacketBuilder::BuildMakeMyCharacterPacket(entityId, pos);

	m_pOwner->SendPacket(sessionId, makeMyCharacterPkt);

	
	if (m_gameInfo.m_usRequiredUserCnt == m_pUserManager->GetUserCount())
	{
		m_pGameWorld->Init(m_gameInfo.m_usMaxUserCnt, m_gameInfo.m_usRequiredUserCnt);
	}

	return;
}

void jh_content::GameSystem::HandleLoadCompletedPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleLoadCompletedPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);
		return;
	}

	if (m_gameInfo.m_usRequiredUserCnt == ++m_dwLoadCompletedCnt)
	{
		printf("HandleLoadCompletedPacket : [requiredUserCnt : %hu ]\n", m_gameInfo.m_usMaxUserCnt);

		PacketPtr gameStartNotifyPkt = jh_content::PacketBuilder::BuildGameStartNotifyPacket();

		m_pUserManager->Broadcast(gameStartNotifyPkt);

		m_pGameWorld->StartGame();

	}

	return;
}

void jh_content::GameSystem::HandleMoveStartRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	//jh_network::MoveStartRequestPacket clientRequestPkt;
	Vector3 clientMoveStartPos;
	float clientMoveStartRotY;

	*packet >> clientMoveStartPos >> clientMoveStartRotY;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStartRequestPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStartRequestPacket] - 플레이어가 존재하지 않습니다. SessionId : [%llu] UserID : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	gamePlayer->MoveStart(clientMoveStartPos, clientMoveStartRotY);

	return;
}
void jh_content::GameSystem::HandleMoveStopRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	//jh_network::MoveStopRequestPacket packet;
	Vector3 clientMoveStopPos;
	float clientMoveStopRotY;

	*packet >> clientMoveStopPos >> clientMoveStopRotY;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStopRequestPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStopRequestPacket] - 플레이어가 존재하지 않습니다. SessionId : [%llu] UserID : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	gamePlayer->MoveStop(clientMoveStopPos, clientMoveStopRotY);

	return;
}

void jh_content::GameSystem::HandleChatRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	// GameWorld Chat
	// roomNum은 무시하도록 한다.
	USHORT roomNum;
	USHORT messageLen;

	*packet >> roomNum >> messageLen;

	char* message = static_cast<char*>(g_memSystem->Alloc(messageLen));

	packet->GetData(message, messageLen);

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatRequestPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);
	}
	else
	{
		ULONGLONG userId = userPtr->GetUserId();

		PacketPtr chatNotifyPacket = jh_content::PacketBuilder::BuildChatNotifyPacket(userId, messageLen, message);

		m_pUserManager->Broadcast(chatNotifyPacket);
	}
	g_memSystem->Free(message);

	return;
}
void jh_content::GameSystem::HandleAttackRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	//jh_network::AttackRequestPacket packet;
	
	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleAttackRequestPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleAttackRequestPacket] - 플레이어가 존재하지 않습니다. SessionId : [%llu] UserID : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	m_pGameWorld->ProcessAttack(gamePlayer);

	return;
}


// 처음 시작할 때 Lan 연결 후 게임과 관련된 정보를 요청하는 패킷에 대한 답장이 왔을 때 답장을 처리하는 함수. Lan 에서 넘겨준다.

void jh_content::GameSystem::HandleGameServerSettingResponsePacket(ULONGLONG lanSessionId, PacketPtr& packets, jh_network::IocpClient* lanClient)
{
	//jh_network::GameServerSettingResponsePacket responsePacket;

	USHORT roomNum;
	USHORT requiredUsers;
	USHORT maxUsers;

	*packets >> roomNum >> requiredUsers >> maxUsers;

	printf("방 번호 [%d]\n", roomNum);
	printf("인원	[%d]\n", requiredUsers);
	printf("제한 인원  [%d]\n", maxUsers);

	// G
	SetGameInfo(roomNum, requiredUsers, maxUsers);

	ULONGLONG xorAfterValue = GetToken() ^ xorTokenKey;

	if (false == m_pOwner->InitSessionArray(maxUsers))
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleGameServerSettingResponsePacket] - maxSession 초기화 실패");
		jh_utility::CrashDump::Crash();
	}

	// TODO : 서버가 0.0.0.0으로 설정되었을 때 ip,port값을 제대로 얻어올 수 있도록 세팅 필요.
	const std::wstring ip = m_pOwner->GetIp();
	USHORT port = m_pOwner->GetPort();

	PacketPtr gameServerLanInfoPkt = jh_content::PacketBuilder::BuildGameServerLanInfoPacket(ip.c_str(), port, roomNum, m_gameInfo.m_ullEnterToken);

	lanClient->SendPacket(lanSessionId, gameServerLanInfoPkt);
	
	printf("ClientJoined Success\n");

	return;
}


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
