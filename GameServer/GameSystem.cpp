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
	
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, GameSystem::StaticLogicProxy, (LPVOID)this, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameSystem Init] - m_hLogicThread ���� ����");
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

			_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameSystem - Stop] ���� ������ ���� ���� GetLastError : [%u]", getLastError);

			jh_utility::CrashDump::Crash();
		}

		CloseHandle(m_hLogicThread);

		m_hLogicThread = nullptr;
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

	std::random_device rd; // ���� ������
	std::mt19937_64 generator(rd()); // �õ� ����.
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


unsigned jh_content::GameSystem::StaticLogicProxy(LPVOID lparam)
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
	// ���� �ٲٱ�.
	while (true == m_bIsRunning.load())
	{
		float deltaTime = timer.Lap<float>();

		if (deltaTime > limitDeltaTime)
			deltaTime = limitDeltaTime;


		// ��Ŷ ó��.
		ProcessNetJob();

		// ���� ó��.
		ProcessSessionConnectionEvent();

		// Lan ��û ó��.
		ProcessLanRequest();

		// Ÿ�̸� �۾� ó��.
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
	static alignas(64) std::vector<JobPtr> jobList;

	m_netJobQueue.PopAll(jobList);

	for (JobPtr& job : jobList)
	{
		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
	}
	jobList.clear();
}

void jh_content::GameSystem::ProcessSessionConnectionEvent()
{
	static alignas(64) std::vector<SessionConnectionEventPtr> sessionConnEventList;

	m_sessionConnEventQueue.PopAll(sessionConnEventList);

	for (SessionConnectionEventPtr& sessionConnEvent : sessionConnEventList)
	{
		ULONGLONG sessionId = sessionConnEvent->m_ullSessionId;
		switch (sessionConnEvent->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{
			// m_pUserManager->CreateGuest(sessionId);
			//m_pUserManager->CreateUser(job->m_llSessionId);
			// ���� ���� ó��.
		}
		break;
		case jh_utility::SessionConnectionEventType::DISCONNECT:
		{
			UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

			if (nullptr == userPtr)
			{
				_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[ProcessSessionConnectionEvent] ������ �������� �ʽ��ϴ�. SessionId: [%llu]", sessionId);
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
			// ���� ���� ���� ó��.
		}
		break;
		default:break;
		}
	}

	sessionConnEventList.clear();
}

void jh_content::GameSystem::ProcessLanRequest()
{
	static alignas(64) std::vector<GameLanRequestPtr> gameLanRequestJobList;

	m_gameLanRequestQueue.PopAll(gameLanRequestJobList);

	for (GameLanRequestPtr& req : gameLanRequestJobList)
	{
		if (req->m_ullSessionId == INVALID_SESSION_ID)
			continue;

		switch (req->m_usMsgType)
		{
		case jh_network::GAME_SERVER_SETTING_RESPONSE_PACKET:HandleGameServerSettingResponsePacket(req->m_ullSessionId, req->m_pPacket, req->m_pClient); break;
			// �߰�

		default:break;
		}
	}

	gameLanRequestJobList.clear();
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

	// TODO : 0916 ���������� ������ ������ �ϴ� Ŭ������ sessionid, userid, player�� �����ؾ��ϴ� ����� ���ؼ� ����
	UserPtr newUser = m_pUserManager->CreateUser(sessionId, userId);
	
	if (nullptr == newUser)
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[HandleEnterGameRequestPacket] - ���� �Ǵ� ������ �ߺ����� ���� ������ �������� �ʾҽ��ϴ�.", userId);
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
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleLoadCompletedPacket] - ������ �������� �ʽ��ϴ�. SessionId : [%llu]", sessionId);
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
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStartRequestPacket] - ������ �������� �ʽ��ϴ�. SessionId : [%llu]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStartRequestPacket] - �÷��̾ �������� �ʽ��ϴ�. SessionId : [%llu] UserID : [%llu]", sessionId, userPtr->GetUserId());

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
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStopRequestPacket] - ������ �������� �ʽ��ϴ�. SessionId : [%llu]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMoveStopRequestPacket] - �÷��̾ �������� �ʽ��ϴ�. SessionId : [%llu] UserID : [%llu]", sessionId, userPtr->GetUserId());

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
	// roomNum�� �����ϵ��� �Ѵ�.
	USHORT roomNum;
	USHORT messageLen;

	*packet >> roomNum >> messageLen;

	char* message = static_cast<char*>(g_memAllocator->Alloc(messageLen));

	packet->GetData(message, messageLen);

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatRequestPacket] - ������ �������� �ʽ��ϴ�. SessionId : [%llu]", sessionId);
	}
	else
	{
		ULONGLONG userId = userPtr->GetUserId();

		PacketPtr chatNotifyPacket = jh_content::PacketBuilder::BuildChatNotifyPacket(userId, messageLen, message);

		m_pUserManager->Broadcast(chatNotifyPacket);
	}
	g_memAllocator->Free(message);

	return;
}
void jh_content::GameSystem::HandleAttackRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	//jh_network::AttackRequestPacket packet;
	
	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleAttackRequestPacket] - ������ �������� �ʽ��ϴ�. SessionId : [%llu]", sessionId);

		return;
	}

	GamePlayerPtr gamePlayer = userPtr->GetPlayer();

	if (nullptr == gamePlayer)
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleAttackRequestPacket] - �÷��̾ �������� �ʽ��ϴ�. SessionId : [%llu] UserID : [%llu]", sessionId, userPtr->GetUserId());

		return;
	}

	if (true == gamePlayer->IsDead())
		return;

	m_pGameWorld->ProcessAttack(gamePlayer);

	return;
}


// ó�� ������ �� Lan ���� �� ���Ӱ� ���õ� ������ ��û�ϴ� ��Ŷ�� ���� ������ ���� �� ������ ó���ϴ� �Լ�. Lan ���� �Ѱ��ش�.

void jh_content::GameSystem::HandleGameServerSettingResponsePacket(ULONGLONG lanSessionId, PacketPtr& packets, jh_network::IocpClient* lanClient)
{
	//jh_network::GameServerSettingResponsePacket responsePacket;

	USHORT roomNum;
	USHORT requiredUsers;
	USHORT maxUsers;

	*packets >> roomNum >> requiredUsers >> maxUsers;

	printf("�� ��ȣ [%d]\n", roomNum);
	printf("�ο�	[%d]\n", requiredUsers);
	printf("���� �ο�  [%d]\n", maxUsers);

	// G
	SetGameInfo(roomNum, requiredUsers, maxUsers);

	ULONGLONG xorAfterValue = GetToken() ^ xorTokenKey;

	if (false == m_pOwner->InitSessionArray(maxUsers))
	{
		_LOG(GAME_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleGameServerSettingResponsePacket] - maxSession �ʱ�ȭ ����");
		jh_utility::CrashDump::Crash();
	}

	// TODO : ������ 0.0.0.0���� �����Ǿ��� �� ip,port���� ����� ���� �� �ֵ��� ���� �ʿ�.
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
