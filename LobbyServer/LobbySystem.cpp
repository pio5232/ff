#include "pch.h"
#include "LobbySystem.h"
#include "RoomManager.h"
#include "UserManager.h"
#include "NetworkBase.h"
#include "User.h"
#include "Memory.h"

void jh_content::LobbySystem::Init()
{
	// m_pRoomManager->Init();

	m_pUserManager->Init();

	m_packetFuncDic.clear();

	m_packetFuncDic[jh_network::ROOM_LIST_REQUEST_PACKET] = &LobbySystem::HandleRoomListRequestPacket;
	m_packetFuncDic[jh_network::CHAT_TO_ROOM_REQUEST_PACKET] = &LobbySystem::HandleChatToRoomRequestPacket;
	m_packetFuncDic[jh_network::LOG_IN_REQUEST_PACKET] = &LobbySystem::HandleLogInRequestPacket;
	m_packetFuncDic[jh_network::MAKE_ROOM_REQUEST_PACKET] = &LobbySystem::HandleMakeRoomRequestPacket;
	m_packetFuncDic[jh_network::ENTER_ROOM_REQUEST_PACKET] = &LobbySystem::HandleEnterRoomRequestPacket;
	m_packetFuncDic[jh_network::LEAVE_ROOM_REQUEST_PACKET] = &LobbySystem::HandleLeaveRoomRequestPacket;
	m_packetFuncDic[jh_network::GAME_READY_REQUEST_PACKET] = &LobbySystem::HandleGameReadyRequestPacket;
	m_packetFuncDic[jh_network::HEART_BEAT_PACKET] = &LobbySystem::HandleHeartbeatPacket;

	m_hJobEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == m_hJobEvent)
	{
		DWORD lastError = GetLastError();
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Lobby System] - JobEvent 핸들이 존재하지 않습니다. 에러 코드 : [%u]",lastError);
		
		jh_utility::CrashDump::Crash();
	}

	LPVOID param = this;
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LobbySystem::StaticLogicProxy, param, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Lobby System] - 로직 핸들이 존재하지 않습니다.");
	
		jh_utility::CrashDump::Crash();
	}
}

void jh_content::LobbySystem::Stop()
{
	m_bRunningFlag.store(false);
	SetEvent(m_hJobEvent);

	if (nullptr != m_hLogicThread)
	{
		DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

		if (ret != WAIT_OBJECT_0)
		{
			DWORD getLastError = GetLastError();

			_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[LobbySystem - Stop] 로직 스레드 종료 오류 GetLastError : [%u]",getLastError);

			jh_utility::CrashDump::Crash();
		}

		CloseHandle(m_hLogicThread);
		CloseHandle(m_hJobEvent);

		m_hLogicThread = nullptr;
		m_hJobEvent = nullptr;
		
		m_netJobQueue.Clear();
		m_sessionConnEventQueue.Clear();
		m_lanRequestQueue.Clear();
	}

}

jh_content::LobbySystem::LobbySystem(jh_network::IocpServer* owner, USHORT maxRoomCnt, USHORT maxRoomUserCnt) : m_pOwner{ owner }, m_hLogicThread{ nullptr },
m_bRunningFlag{ true }, m_netJobQueue{}, m_sessionConnEventQueue{}, m_hJobEvent{ nullptr }
{
	if (nullptr == m_pOwner)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"LobbySystem() - Owner (Server) is NULL");
		jh_utility::CrashDump::Crash();
	}

	m_pUserManager = std::make_unique<jh_content::UserManager>();
	m_pRoomManager = std::make_unique<jh_content::RoomManager>(maxRoomCnt, maxRoomUserCnt);

	m_pendingGameRoomList.reserve(20);

}

jh_content::LobbySystem::~LobbySystem()
{
	if (false != m_bRunningFlag)
	{
		Stop();
	}
}

unsigned __stdcall jh_content::LobbySystem::StaticLogicProxy(LPVOID lparam)
{
	jh_content::LobbySystem* lobbyInstance = static_cast<LobbySystem*>(lparam);

	if (nullptr == lobbyInstance)
		return 0;

	lobbyInstance->LobbyLogic();

	return 0;
}


void jh_content::LobbySystem::LobbyLogic()
{
	static ULONGLONG lastUpdateTime = jh_utility::GetTimeStamp();

	while (true == m_bRunningFlag)
	{
		WaitForSingleObject(m_hJobEvent, 3000);
		
		// Session 연결에 대한 처리
		ProcessSessionConnectionEvent();

		// Packet 처리
		ProcessNetJob();

		// Lan 요청 처리
		ProcessLanRequest();

		ULONGLONG currentTime = jh_utility::GetTimeStamp();

		if (currentTime - lastUpdateTime > LOBBY_TIMEOUT_CHECK_INTERVAL)
		{
			m_pOwner->CheckHeartbeatTimeout(currentTime);

			lastUpdateTime = currentTime;
		}
		
	}
}

ErrorCode jh_content::LobbySystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return ErrorCode::CANNOT_FIND_PACKET_FUNC;

	return (this->*m_packetFuncDic[packetType])(sessionId, packet);
}

void jh_content::LobbySystem::ProcessNetJob()
{
	static alignas(64) std::vector<JobPtr> jobList;

	m_netJobQueue.PopAll(jobList);

	for (JobPtr& job : jobList)
	{
		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
	}
	jobList.clear();
}

void jh_content::LobbySystem::ProcessSessionConnectionEvent()
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
			//m_pUserManager->CreateUser(job->m_llSessionId);
		}
		break;
		case jh_utility::SessionConnectionEventType::DISCONNECT:
		{
			UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

			if (nullptr == userPtr)
			{
				_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L" [ProcessSessionConnectionEvent] - 유저 정보가 존재하지 않습니다. SessionId : [%llu]", sessionId);

				break;
			}

			auto [isJoined, roomNum] = userPtr->TryGetRoomId();

			if (false == isJoined)
				break;
		
			RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

			if (nullptr == roomPtr)
			{
				_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[ProcessSessionConnectionEvent] - 유저와 RoomManager가 관리하는 방 정보가 다릅니다. 방 번호 : [%hu]", roomNum);
			}
			else
			{
				bool isRoomEmpty = roomPtr->LeaveRoom(userPtr);

				if (isRoomEmpty == true)
					m_pRoomManager->DestroyRoom(roomPtr->GetRoomNum());
			}

			m_pUserManager->RemoveUser(sessionConnEvent->m_ullSessionId);
		}
		break;
		default:break;
		}
	}

	sessionConnEventList.clear();
}

void jh_content::LobbySystem::ProcessLanRequest()
{
	static alignas(64) std::vector<LanRequestPtr> lanRequestJobList;

	m_lanRequestQueue.PopAll(lanRequestJobList);

	for (LanRequestPtr& req : lanRequestJobList)
	{
		if (req->m_ullSessionId == INVALID_SESSION_ID)
			continue;

		switch (req->m_usMsgType)
		{
		case jh_network::GAME_SERVER_SETTING_REQUEST_PACKET: HandleGameSettingRequest(req->m_ullSessionId, req->m_pPacket, req->m_pLanServer); break;
		case jh_network::GAME_SERVER_LAN_INFO_PACKET: HandleLanInfoNotify(req->m_ullSessionId, req->m_pPacket, req->m_pLanServer); break;

		default:break;
		}
	}

	lanRequestJobList.clear();
}

ErrorCode jh_content::LobbySystem::HandleRoomListRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"HandleRoomListRequest for session %lld", sessionId);

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleRoomListRequestPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);

		return ErrorCode::NOT_FOUND;
	}

	std::vector<jh_content::RoomInfo> roomInfos = m_pRoomManager->GetRoomInfos(); // Get list from RoomManager

	PacketPtr responsePacket = PacketBuilder::BuildRoomListResponsePacket(roomInfos);
	
	//std::cout << "[HandleRoomListRequestPacket] send pending Size : " << responsePacket->GetDataSize() << " capa : " << responsePacket->GetBufferSize() << "\n";
	m_pOwner->SendPacket(sessionId, responsePacket);
	
	
	return ErrorCode::NONE;
}

ErrorCode jh_content::LobbySystem::HandleLogInRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	std::cout << "LogIn Request\n";

	jh_network::LogInRequestPacket clientRequestPacket;

	*packet >> clientRequestPacket;

	UserPtr userPtr = m_pUserManager->CreateUser(sessionId);
	
	ULONGLONG userId = userPtr->GetUserId();

	PacketPtr sendBuffer = jh_content::PacketBuilder::BuildLogInResponsePacket(userId);

	m_pOwner->SendPacket(sessionId, sendBuffer);

	return ErrorCode::NONE;
}
ErrorCode jh_content::LobbySystem::HandleChatToRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	USHORT roomNum;
	USHORT messageLen; // WCHAR

	*packet >> roomNum >> messageLen;

	char* payLoad = static_cast<char*>(g_memAllocator->Alloc(messageLen));

	//char* payLoad = static_cast<char*>(malloc(messageLen * MESSAGE_SIZE));

	packet->GetData(payLoad, messageLen);


	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatToRoomRequestPacket] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);
		
		return ErrorCode::NOT_FOUND;
	}

	ULONGLONG userId = userPtr->GetUserId();

	jh_network::PacketHeader packetHeader;

	packetHeader.size = 0;
	packetHeader.type = jh_network::CHAT_TO_ROOM_RESPONSE_PACKET;

	// --- ChatRoomResponsePacket
	PacketPtr responseBuffer = MakeSharedBuffer(g_memAllocator,sizeof(packetHeader));
	*responseBuffer << packetHeader;

	m_pOwner->SendPacket(sessionId, responseBuffer);
	
	// --- NotifyPacket
	packetHeader.size = sizeof(userId) + sizeof(messageLen) + messageLen;
	packetHeader.type = jh_network::CHAT_NOTIFY_PACKET;

	PacketPtr notifyBuffer = MakeSharedBuffer(g_memAllocator, sizeof(packetHeader) + packetHeader.size);
	*notifyBuffer << packetHeader << userId << messageLen;

	notifyBuffer->PutData(reinterpret_cast<const char*>(payLoad), messageLen);

	g_memAllocator->Free(payLoad);

	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	if (nullptr == roomPtr)
		return ErrorCode::ACCESS_DESTROYED_ROOM;

	roomPtr->BroadCast(notifyBuffer);

	return ErrorCode::NONE;
}

ErrorCode jh_content::LobbySystem::HandleMakeRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	wprintf(L" MakeRoom Request Recv\n");

	WCHAR roomName[ROOM_NAME_MAX_LEN]{};

	packet->GetData(reinterpret_cast<char*>(&roomName), sizeof(roomName));

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (userPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L" [HandleMakeRoomRequestPacket] - 유저 정보가 존재하지 않습니다. SessionID : [%llu].", sessionId);

		return ErrorCode::NOT_FOUND;
	}

	RoomPtr newRoomPtr = m_pRoomManager->CreateRoom(userPtr, roomName);

	jh_network::MakeRoomResponsePacket makeRoomResponsePacket;

	if (newRoomPtr == nullptr)
	{
		makeRoomResponsePacket.isMade = false;
		PacketPtr packetBuffer = MakeSharedBuffer(g_memAllocator, sizeof(makeRoomResponsePacket));

		*packetBuffer << makeRoomResponsePacket;

		m_pOwner->SendPacket(sessionId, packetBuffer);
		printf("EnterRoom Response Packet Send -- [bAllow = false]");
		
		return ErrorCode::CREATE_ROOM_FAILED;
	}
	else
	{
		auto sendPacketFunc = [this](ULONGLONG sessionId, PacketPtr& _packet)
			{
				m_pOwner->SendPacket(sessionId, _packet);
			};

		newRoomPtr->SetSendPacketFunc(sendPacketFunc);

		makeRoomResponsePacket.isMade = true;

		makeRoomResponsePacket.roomInfo.m_usCurUserCnt = newRoomPtr->GetCurUserCnt();
		makeRoomResponsePacket.roomInfo.m_usMaxUserCnt = newRoomPtr->GetMaxUserCnt();
		makeRoomResponsePacket.roomInfo.m_ullOwnerId = newRoomPtr->GetOwnerId();
		makeRoomResponsePacket.roomInfo.m_usRoomNum = newRoomPtr->GetRoomNum();
		wmemcpy_s(makeRoomResponsePacket.roomInfo.m_wszRoomName, ROOM_NAME_MAX_LEN, roomName, ROOM_NAME_MAX_LEN);

		PacketPtr packetBuffer = MakeSharedBuffer(g_memAllocator, sizeof(makeRoomResponsePacket));

		*packetBuffer << makeRoomResponsePacket;

		m_pOwner->SendPacket(sessionId, packetBuffer);
	}

	newRoomPtr->TryEnterRoom(userPtr);

	{
		static std::vector<ULONGLONG> emptyVector;

		ULONGLONG userId = userPtr->GetUserId();

		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildEnterRoomResponsePacket(true, emptyVector);

		m_pOwner->SendPacket(sessionId, sendBuffer);
	}

	return ErrorCode::NONE;
}
ErrorCode jh_content::LobbySystem::HandleEnterRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	jh_network::EnterRoomRequestPacket requestPacket;

	*packet >> requestPacket;

	RoomPtr roomPtr = m_pRoomManager->GetRoom(requestPacket.roomNum);

	if (nullptr == roomPtr)
	{
		PacketPtr packetPtr = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::REQUEST_DESTROYED_ROOM);

		m_pOwner->SendPacket(sessionId, packetPtr);

		printf("Invalid Access - Destroyed Room");
		return ErrorCode::CANNOT_FIND_ROOM;
	}

	if (wcscmp(requestPacket.roomName, static_cast<const WCHAR*>(roomPtr->GetRoomNamePtr())) != 0)
	{
		PacketPtr packetPtr = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::REQUEST_DIFF_ROOM_NAME);

		m_pOwner->SendPacket(sessionId, packetPtr);

		printf("EnterRoom - Room name is Diffrent\n");
		return ErrorCode::CANNOT_FIND_ROOM;
	}

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (userPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L" [HandleEnterRoomRequestPacket] - 유저 정보가 존재하지 않습니다. SessionID : [%llu]", sessionId);

		return ErrorCode::NOT_FOUND;
	}

	ULONGLONG userId = userPtr->GetUserId();

	printf("EnterRoom Response Packet Send  ");

	jh_content::Room::RoomEnterResult res = roomPtr->TryEnterRoom(userPtr);

	switch (res)
	{
	case jh_content::Room::RoomEnterResult::SUCCESS:
	{
		{
			PacketPtr sendBuffer = jh_content::PacketBuilder::BuildEnterRoomNotifyPacket(userId);

			roomPtr->BroadCast(sendBuffer, userId);
		}
		
		// 방에 접속한 유저들의 정보를 전달
		std::vector<ULONGLONG> userIdAndReadyList;

		const std::unordered_map<ULONGLONG, std::weak_ptr<User>>& userMap = roomPtr->GetUserMap();

		userIdAndReadyList.reserve(userMap.size());

		for (const auto& [_id, _userWptr] : userMap)
		{
			if (_id == userId)
				continue;

			UserPtr _userPtr = _userWptr.lock();

			if (_userPtr == nullptr)
				continue;

			ULONGLONG idAndReadyState = _id | ((ULONGLONG)userPtr->GetReadyState() << 63);

			userIdAndReadyList.emplace_back(idAndReadyState);
		}

		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildEnterRoomResponsePacket(true, userIdAndReadyList);
		m_pOwner->SendPacket(sessionId, sendBuffer);

		break;
	}
	case jh_content::Room::RoomEnterResult::FULL:
	{
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::FULL_ROOM);

		m_pOwner->SendPacket(sessionId, sendBuffer);

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleEnterRoomRequestPacket] - 인원이 가득 차서 참가할 수 없습니다. UserId : [%llu]",userId);
		
		break;
	}
	case jh_content::Room::RoomEnterResult::ALREADY_STARTED:
	{
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::ALREADY_RUNNING_ROOM);

		m_pOwner->SendPacket(sessionId, sendBuffer);

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleEnterRoomRequestPacket] - 이미 게임을 시작한 방입니다. UserId : [%llu]",userId);
		
		break;
	}
	}
	return ErrorCode::NONE;
}
ErrorCode jh_content::LobbySystem::HandleLeaveRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	jh_network::LeaveRoomRequestPacket leaveRoomRequestPacket;

	*packet >> leaveRoomRequestPacket;

	RoomPtr roomPtr = m_pRoomManager->GetRoom(leaveRoomRequestPacket.roomNum);

	if (roomPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L" [HandleLeaveRoomRequestPacket] - 존재하지 않는 방입니다. Room : [%hu], SessionId : [%llu]",leaveRoomRequestPacket.roomNum, sessionId);

		return ErrorCode::ACCESS_DESTROYED_ROOM;
	}

	if (wcscmp(leaveRoomRequestPacket.roomName, static_cast<const WCHAR*>(roomPtr->GetRoomNamePtr())) != 0)
	{
		printf("LeaveRoom - Room name is Diffrent\n");
		return ErrorCode::CANNOT_FIND_ROOM;
	}

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (userPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L" [HandleLeaveRoomRequestPacket] - 유저 정보가 존재하지 않습니다. SessionId : [%llu]", sessionId);

		return ErrorCode::NOT_FOUND;
	}

	bool isRoomEmpty = roomPtr->LeaveRoom(userPtr);

	if (isRoomEmpty == true)
		m_pRoomManager->DestroyRoom(roomPtr->GetRoomNum());

	return ErrorCode::NONE;
}

ErrorCode jh_content::LobbySystem::HandleGameReadyRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	jh_network::GameReadyRequestPacket requestPacket;

	*packet >> requestPacket.isReady;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	// �濡 �������� ���� �����ε� �غ� ��Ŷ�� ����.
	if (!userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"-[GameReadyRequest] -  유저 정보가 존재하지 않습니다. SessionID : [%llu]", sessionId);
		return ErrorCode::NOT_FOUND;
	}

	auto [res, roomNum] = userPtr->TryGetRoomId();

	if (res == false)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameReadyRequest] - 방에 접속하지 않은 유저의 준비패킷입니다. UserID : [%llu]", userPtr->GetUserId());
		
		return ErrorCode::NOT_FOUND;
	}
	
	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	bool canStart = roomPtr->UpdateUserReadyStatus(userPtr, requestPacket.isReady, true);

	if (canStart == true)
	{
		//std::wstring path(L"E:\\GameServer\\x64\\Debug\\GameServer.exe");
		//$(SolutionDir)Exe\$(Configuration)
		//std::wstring path(L"$(SolutionDir)Exe\\$(Configuration)\\GameServer.exe");
		std::wstring path(GAME_FILE_PATH);


		//std::wstring args(std::to_wstring(m_usRoomNumber) + L" " + std::to_wstring(GetCurUserCnt()) + L" " + std::to_wstring(GetMaxUserCnt()));
		//wprintf(L"path : %s    args : %s", path, args);
		ExecuteProcess(GAME_FILE_PATH, GAME_CUR_DIRECTORY);

		USHORT gameRoomNum = roomPtr->GetRoomNum();
		
		m_pendingGameRoomList.emplace_back(gameRoomNum);
		
		// ������ �������� �� �� ������ ��� �� �� �����ؾ��Ѵ�.

	}
}

ErrorCode jh_content::LobbySystem::HandleHeartbeatPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	ULONGLONG packetTimeStamp;

	*packet >> packetTimeStamp;
	printf("SessionId : %llu, [comTimeStamp : %llu], [PacketTimeStamp : %llu]\n", sessionId, jh_utility::GetTimeStamp(), packetTimeStamp);

	m_pOwner->UpdateHeartbeat(sessionId, packetTimeStamp);

	return ErrorCode::NONE;
}

void jh_content::LobbySystem::HandleLanInfoNotify(ULONGLONG lanSessionId, PacketPtr& lanPacket, jh_network::IocpServer* lanServer)
{
	printf("Lan Info Notify Packet Recv\n");

	WCHAR ipStr[IP_STRING_LEN] = {};
	USHORT port = 0;
	USHORT roomNum = 0;
	ULONGLONG xorToken = 0;

	lanPacket->GetData(reinterpret_cast<char*>(ipStr), sizeof(ipStr));

	*lanPacket >> port >> roomNum >> xorToken;

	wprintf(L"ip: [ %s ]\n", ipStr);
	wprintf(L"port : [ %d ]\n", port);
	wprintf(L"Room : [ %d ]\n", roomNum);
	wprintf(L"xorToken : [ %llu ], After : [%llu]\n", xorToken, xorToken ^ xorTokenKey);

	PacketPtr sendBuffer = jh_content::PacketBuilder::BuildLanInfoPacket(ipStr, port, roomNum, xorToken ^ xorTokenKey);// C_Network::BufferMaker::MakeSendBuffer(sizeof(lanInfoPacket));

	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	if (roomPtr == nullptr)
	{
		// LOG
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"-[HandleLanInfoNotify] - ��û�� ���� ������ �������� �ʽ��ϴ�.", roomNum);

		return;
	}

	roomPtr->BroadCast(sendBuffer);
}

void jh_content::LobbySystem::HandleGameSettingRequest(ULONGLONG lanSessionId, PacketPtr& lanPacket, jh_network::IocpServer* lanServer)
{
	printf("Game Setting Request Packet Recv\n");

	if (m_pendingGameRoomList.empty())
		return;

	USHORT roomNum = m_pendingGameRoomList.back();
	m_pendingGameRoomList.pop_back();

	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	if (roomPtr == nullptr)
	{
		// LOG
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"-[HandleGameSettingRequest] - ��û�� ���� ������ �������� �ʽ��ϴ�.", roomNum);
		return;
	}

	USHORT requiredUserCnt = roomPtr->GetCurUserCnt();
	USHORT maxUserCnt = roomPtr->GetMaxUserCnt();

	PacketPtr sendBuffer = jh_content::PacketBuilder::BuildGameServerSettingResponsePacket(roomNum, requiredUserCnt, maxUserCnt);

	lanServer->SendPacket(lanSessionId, sendBuffer);
}
