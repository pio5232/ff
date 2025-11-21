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
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Init] Logic thread creation failed.");
		jh_utility::CrashDump::Crash();
	}
}

void jh_content::GameSystem::Stop()
{
	InterlockedExchange8(&m_bIsRunning, 0);

	if (nullptr != m_hLogicThread)
	{
		DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

		if (ret != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();

			_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Stop] Logic thread wait failed. GetLastError : [%u]", gle);

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

jh_content::GameSystem::GameSystem(jh_network::IocpServer* owner) : m_hLogicThread{ nullptr }, m_bIsRunning{ 0 }, m_usLoadCompletedCnt{ 0 }, m_pOwner{ owner }, m_gameInfo{}
{
	auto sendPacketFunc = [this](ULONGLONG sessionId, PacketBufferRef& _packet)
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
	if (0 != InterlockedOr8(&m_bIsRunning, 0))
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

	InterlockedExchange8(&gameInstance->m_bIsRunning, 1);
	gameInstance->GameLogic();

	return 0;
}

void jh_content::GameSystem::GameLogic()
{
	jh_utility::Timer timer;

	timer.Start();

	float deltaSum = 0;
	// 조건 바꾸기.
	while (1 == InterlockedOr8(&m_bIsRunning, 0))
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
}

void jh_content::GameSystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketBufferRef& packet)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return;

	(this->*m_packetFuncDic[packetType])(sessionId, packet);
	
	return;
}

void jh_content::GameSystem::ProcessNetJob()
{
	static thread_local alignas(64) std::queue<JobRef> gameJobQ;
	std::queue<JobRef>	emptyQ;

	m_netJobQueue.Swap(gameJobQ);

	while(gameJobQ.size() > 0)
	{
		JobRef& job = gameJobQ.front();

		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
		
		gameJobQ.pop();
	}

	gameJobQ.swap(emptyQ);
}

void jh_content::GameSystem::ProcessSessionConnectionEvent()
{
	static alignas(64) std::queue<SessionConnectionEventRef> sessionConnEventQ;
	std::queue<SessionConnectionEventRef> emptyQ;

	m_sessionConnEventQueue.Swap(sessionConnEventQ);

	while(sessionConnEventQ.size() > 0)
	{
		SessionConnectionEventRef& sessionConnEvent = sessionConnEventQ.front();

		ULONGLONG sessionId = sessionConnEvent->m_ullSessionId;
		switch (sessionConnEvent->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{

		}
		break;
		case jh_utility::SessionConnectionEventType::DISCONNECT:
		{
			UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

			if (nullptr == userPtr)
			{
				_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[ProcessSessionConnectionEvent] User not found on disconnect. SessionId: [0x%016llx]", sessionId);
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

void jh_content::GameSystem::HandleEnterGameRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	ULONGLONG receivedUserId;
	ULONGLONG token;
	*packet >> receivedUserId >> token;

	if (m_gameInfo.m_ullEnterToken != token)
	{
		PacketBufferRef errPkt = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::CONNECTED_FAILED_WRONG_TOKEN);

		m_pOwner->SendPacket(sessionId, errPkt);

		return;
	}

	UserPtr newUser = m_pUserManager->CreateUser(sessionId, receivedUserId);
	
	if (nullptr == newUser)
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[HandleEnterGameRequestPacket] User creation failed Received UserId : [%llu]", receivedUserId);
		return;
	}
	PacketBufferRef enterGameResponsePkt = jh_content::PacketBuilder::BuildEnterGameResponsePacket();
	m_pOwner->SendPacket(sessionId, enterGameResponsePkt);

	GamePlayerPtr newPlayer = m_pGameWorld->CreateGamePlayer(newUser);

	newUser->SetPlayer(newPlayer);

	m_pGameWorld->AddEntity(static_pointer_cast<jh_content::Entity>(newPlayer));
	m_pUserManager->RegisterEntityIdToUser(newPlayer->GetEntityId(), newUser);
	
	jh_network::MakeMyCharacterPacket makeMyCharacterPacket;
	
	ULONGLONG entityId = newPlayer->GetEntityId();
	const Vector3& pos = newPlayer->GetPosition();

	PacketBufferRef makeMyCharacterPkt = jh_content::PacketBuilder::BuildMakeMyCharacterPacket(entityId, pos);

	m_pOwner->SendPacket(sessionId, makeMyCharacterPkt);

	if (m_gameInfo.m_usRequiredUserCnt == m_pUserManager->GetUserCount())
	{
		m_pGameWorld->Init(m_gameInfo.m_usMaxUserCnt, m_gameInfo.m_usRequiredUserCnt);
	}

	return;
}

void jh_content::GameSystem::HandleLoadCompletedPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleLoadCompletedPacket] User not found. SessionId : [0x%016llx]", sessionId);
		return;
	}

	_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleLoadCompletedPacket] User Load Completed, SessionId : [0x%016llx], UserId : [%llu]", sessionId,userPtr->GetUserId());

	if (m_gameInfo.m_usRequiredUserCnt == ++m_usLoadCompletedCnt)
	{
		PacketBufferRef gameStartNotifyPkt = jh_content::PacketBuilder::BuildGameStartNotifyPacket();

		m_pUserManager->Broadcast(gameStartNotifyPkt);

		m_pGameWorld->StartGame();
	}

	return;
}

void jh_content::GameSystem::HandleMoveStartRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	Vector3 clientMoveStartPos;
	float clientMoveStartRotY;

	*packet >> clientMoveStartPos >> clientMoveStartRotY;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStartRequestPacket] User not found. SessionId : [0x%016llx]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStartRequestPacket] Player not found. SessionId : [0x%016llx], UserId : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	gamePlayer->MoveStart(clientMoveStartPos, clientMoveStartRotY);

	return;
}
void jh_content::GameSystem::HandleMoveStopRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	Vector3 clientMoveStopPos;
	float clientMoveStopRotY;

	*packet >> clientMoveStopPos >> clientMoveStopRotY;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStopRequestPacket] User not found. SessionId : [0x%016llx]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStopRequestPacket] Player not found. SessionId : [%0x016llx], UserID : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	gamePlayer->MoveStop(clientMoveStopPos, clientMoveStopRotY);

	return;
}

void jh_content::GameSystem::HandleChatRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	// GameWorld Chat
	// roomNum은 무시한다.
	USHORT roomNum;
	USHORT messageLen;

	*packet >> roomNum >> messageLen;

	char* message = static_cast<char*>(g_memSystem->Alloc(messageLen));

	packet->GetData(message, messageLen);

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatRequestPacket] User not found. SessionId : [0x%016llx]", sessionId);
	}
	else
	{
		ULONGLONG userId = userPtr->GetUserId();

		PacketBufferRef chatNotifyPacket = jh_content::PacketBuilder::BuildChatNotifyPacket(userId, messageLen, message);

		m_pUserManager->Broadcast(chatNotifyPacket);
	}
	g_memSystem->Free(message);

	return;
}
void jh_content::GameSystem::HandleAttackRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{	
	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleAttackRequestPacket] User not found. SessionId : [0x%016llx]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleAttackRequestPacket] Player not found. SessionId : [0x%016llx], UserID : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	m_pGameWorld->ProcessAttack(gamePlayer);

	return;
}


// 처음 시작할 때 Lan 연결 후 게임과 관련된 정보를 요청하는 패킷에 대한 답장이 왔을 때 답장을 처리하는 함수. Lan 에서 넘겨준다.
void jh_content::GameSystem::HandleGameServerSettingResponsePacket(ULONGLONG lanSessionId, PacketBufferRef& packets, jh_network::IocpClient* lanClient)
{

	USHORT roomNum;
	USHORT requiredUsers;
	USHORT maxUsers;

	*packets >> roomNum >> requiredUsers >> maxUsers;

	SetGameInfo(roomNum, requiredUsers, maxUsers);

	ULONGLONG xorAfterValue = GetToken() ^ xorTokenKey;

	if (false == m_pOwner->InitSessionArray(maxUsers))
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleGameServerSettingResponsePacket] InitSessionArray failed.");
		jh_utility::CrashDump::Crash();
	}

	const std::wstring ip = m_pOwner->GetIp();
	USHORT port = m_pOwner->GetPort();

	PacketBufferRef gameServerLanInfoPkt = jh_content::PacketBuilder::BuildGameServerLanInfoPacket(ip.c_str(), port, roomNum, m_gameInfo.m_ullEnterToken);

	lanClient->SendPacket(lanSessionId, gameServerLanInfoPkt);

	_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleGameServerSettingResponsePacket] roomNum : [%hd], requiredUsers : [%hd], maxUsers : [%hd] Client Joined", roomNum,requiredUsers, maxUsers);

	return;
}
