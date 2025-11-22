#include "pch.h"
#include "LobbySystem.h"
#include "RoomManager.h"
#include "UserManager.h"
#include "NetworkBase.h"
#include "User.h"

void jh_content::LobbySystem::Init()
{
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
	m_packetFuncDic[jh_network::ECHO_PACKET] = &LobbySystem::HandleEchoPacket;

	m_hJobEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == m_hJobEvent)
	{
		DWORD gle = GetLastError();
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[LobbySystem] JobEvent handle is nullptr. GetLastError: [%u]", gle);
		jh_utility::CrashDump::Crash();
	}

	LPVOID param = this;
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LobbySystem::LogicThreadFunc, param, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[LobbySystem] Logic thread handle is nullptr.");

		jh_utility::CrashDump::Crash();
	}
}

void jh_content::LobbySystem::Stop()
{
	InterlockedExchange8(&m_bRunningFlag, 0);

	SetEvent(m_hJobEvent);

	if (nullptr != m_hLogicThread)
	{
		DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

		if (ret != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();

			_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Stop] Logic thread wait failed. GetLastError : [%u]", gle);

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

void jh_content::LobbySystem::GetInvalidMsgCnt()
{
	wprintf(L" [Content] Invliad Leave : %lld\n", m_invalidLeave);
	wprintf(L" [Content] Invalid Enter : %lld\n", m_invalidEnter);
	wprintf(L" [Content] Invalid Chat : %lld\n", m_invalidChat);
	wprintf(L" [Content] Invalid Make : %lld\n", m_invalidMake);
	wprintf(L" [Content] Invalid Ready : %lld\n", m_invalidReady);

}

jh_content::LobbySystem::LobbySystem(jh_network::IocpServer* owner, USHORT maxRoomCnt, USHORT maxRoomUserCnt) : m_pOwner{ owner }, m_hLogicThread{ nullptr },
m_bRunningFlag{ true }, m_netJobQueue{}, m_sessionConnEventQueue{}, m_hJobEvent{ nullptr }
{
	if (nullptr == m_pOwner)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[LobbySystem] m_pOwner is nullptr");
		jh_utility::CrashDump::Crash();
	}

	m_pUserManager = std::make_unique<jh_content::UserManager>();
	m_pRoomManager = std::make_unique<jh_content::RoomManager>(maxRoomCnt, maxRoomUserCnt);

	m_pendingGameRoomList.reserve(20);

}

jh_content::LobbySystem::~LobbySystem()
{
	if (0 != InterlockedOr8(&m_bRunningFlag, 0))
	{
		Stop();
	}
}

unsigned __stdcall jh_content::LobbySystem::LogicThreadFunc(LPVOID lparam)
{
	jh_content::LobbySystem* lobbyInstance = static_cast<LobbySystem*>(lparam);

	if (nullptr == lobbyInstance)
		return 0;

	lobbyInstance->LogicThreadMain();

	return 0;
}


void jh_content::LobbySystem::LogicThreadMain()
{
	static ULONGLONG lastUpdateTime = jh_utility::GetTimeStamp();

	while (1 == InterlockedOr8(&m_bRunningFlag, 0))
	{
		WaitForSingleObject(m_hJobEvent, 1000);
		
		// Session 연결에 대한 처리
		ProcessSessionConnectionEvent();

		// Packet 처리
		ProcessNetJob();

		// Lan 요청 처리
		ProcessLanRequest();

		ULONGLONG currentTime = jh_utility::GetTimeStamp();

		if ((currentTime - lastUpdateTime) > LOBBY_TIMEOUT_CHECK_INTERVAL)
		{
			m_pOwner->CheckHeartbeatTimeout(currentTime);

			lastUpdateTime = currentTime;
		}
		
	}
}

void jh_content::LobbySystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketBufferRef& packet)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return;

	(this->*m_packetFuncDic[packetType])(sessionId, packet);
}

void jh_content::LobbySystem::ProcessNetJob()
{
	PRO_START_AUTO_FUNC;
	static alignas(64) std::queue<JobRef> lobbyJobQ;

	m_netJobQueue.Swap(lobbyJobQ);

	_LOG(L"Job", LOG_LEVEL_SYSTEM, L"[ProcessNetJob] Job Count : [%d]", lobbyJobQ.size());

	while(lobbyJobQ.size() > 0)
	{
		JobRef& job = lobbyJobQ.front();

		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
		
		lobbyJobQ.pop();
	}
}

void jh_content::LobbySystem::ProcessSessionConnectionEvent()
{
	static alignas(64) std::queue<SessionConnectionEventRef> sessionConnEventJob;

	m_sessionConnEventQueue.Swap(sessionConnEventJob);

	while(sessionConnEventJob.size() > 0)
	{
		SessionConnectionEventRef& sessionConnEvent = sessionConnEventJob.front();
		
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
				_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[ProcessSessionConnectionEvent] User not found on disconnect. SessionId: [0x%016llx]", sessionId);
				break;
			}

			auto [isJoined, roomNum] = userPtr->TryGetRoomId();

			if (true == isJoined)
			{
				RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

				if (nullptr == roomPtr)
				{
					_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[ProcessSessionConnectionEvent] User room mismatch. RoomNum: [%hu]", roomNum);
				}
				else
				{
					bool isRoomEmpty = roomPtr->LeaveRoom(userPtr);

					if (isRoomEmpty == true)
						m_pRoomManager->DestroyRoom(roomPtr->GetRoomNum());
				}
			}

			m_pUserManager->RemoveUser(sessionConnEvent->m_ullSessionId);
		}
		break;
		default:break;
		}

		sessionConnEventJob.pop();
	}
}

void jh_content::LobbySystem::ProcessLanRequest()
{
	static alignas(64) std::queue<LanRequestPtr> lanRequestJobQ;

	m_lanRequestQueue.Swap(lanRequestJobQ);
	
	while(lanRequestJobQ.size() > 0)
	{
		LanRequestPtr& req = lanRequestJobQ.front();

		switch (req->m_usMsgType)
		{
		case jh_network::GAME_SERVER_SETTING_REQUEST_PACKET: HandleGameSettingRequest(req->m_ullSessionId, req->m_pPacket, req->m_pLanServer); break;
		case jh_network::GAME_SERVER_LAN_INFO_PACKET: HandleLanInfoNotify(req->m_ullSessionId, req->m_pPacket, req->m_pLanServer); break;

		default:break;
		}

		lanRequestJobQ.pop();
	}

}

void jh_content::LobbySystem::HandleRoomListRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleRoomListRequestPacket] User not found. SessionId : [0x%016llx]", sessionId);

		return;
	}

	std::vector<jh_content::RoomInfo> roomInfos = m_pRoomManager->GetRoomInfos(); // Get list from RoomManager

	PacketBufferRef responsePkt = PacketBuilder::BuildRoomListResponsePacket(roomInfos);
	
	m_pOwner->SendPacket(sessionId, responsePkt);
}

void jh_content::LobbySystem::HandleLogInRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	jh_network::LogInRequestPacket clientRequestPacket;

	*packet >> clientRequestPacket;

	UserPtr userPtr = m_pUserManager->CreateUser(sessionId);
	
	ULONGLONG userId = userPtr->GetUserId();

	PacketBufferRef logInResponsePkt = jh_content::PacketBuilder::BuildLogInResponsePacket(userId);

	m_pOwner->SendPacket(sessionId, logInResponsePkt);
}
void jh_content::LobbySystem::HandleChatToRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	USHORT messageLen; // WCHAR
	USHORT roomNum;

	*packet >> roomNum >> messageLen;
	
	std::shared_ptr<char> payLoad(static_cast<char*>(g_memSystem->Alloc(messageLen)),[](char* p) { g_memSystem->Free(p); });
	
	packet->GetData(payLoad.get(), messageLen);

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatToRoomRequestPacket] User not found. SessionId : [0x%016llx]", sessionId);

		return;
	}

	auto [isAlreadyInRoom, serverRoomNum] = userPtr->TryGetRoomId();
	ULONGLONG userId = userPtr->GetUserId();

	if (false == isAlreadyInRoom || serverRoomNum != roomNum)
	{
		m_invalidChat++;

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatToRoomRequestPacket] Invalid room. UserId : [%llu], ClientRoom : [%hu], ServerRoom: [%hu]", userId, roomNum, serverRoomNum);

		return;
	}

	jh_network::PacketHeader packetHeader;

	packetHeader.size = 0;
	packetHeader.type = jh_network::CHAT_TO_ROOM_RESPONSE_PACKET;

	// --- ChatRoomResponsePacket
	PacketBufferRef chatResponsePkt = jh_memory::MakeShared<PacketBuffer>(sizeof(packetHeader));
	*chatResponsePkt << packetHeader;
	
	m_pOwner->SendPacket(sessionId, chatResponsePkt);
	
	// --- NotifyPacket
	packetHeader.size = sizeof(userId) + sizeof(messageLen) + messageLen;
	packetHeader.type = jh_network::CHAT_NOTIFY_PACKET;

	PacketBufferRef notifyBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(packetHeader) + packetHeader.size);
	*notifyBuffer << packetHeader << userId << messageLen;

	notifyBuffer->PutData(reinterpret_cast<const char*>(payLoad.get()), messageLen);

	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	if (nullptr == roomPtr)
		return;;

	roomPtr->BroadCast(notifyBuffer);
}

void jh_content::LobbySystem::HandleMakeRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	WCHAR roomName[ROOM_NAME_MAX_LEN]{};

	packet->GetData(reinterpret_cast<char*>(&roomName), sizeof(roomName));

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleMakeRoomRequestPacket] User not found. SessionID : [0x%016llx].", sessionId);

		return;
	}

	auto [isAlreadyInRoom, roomNum] = userPtr->TryGetRoomId();
	
	if (true == isAlreadyInRoom)
	{
		m_invalidMake++;
		
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleMakeRoomRequestPacket] User already in room. SessionId : [0x%016llx], ServerRoom: [%hu]", sessionId, roomNum);

		return;
	}

	RoomPtr newRoomPtr = m_pRoomManager->CreateRoom(userPtr, roomName);

	jh_network::MakeRoomResponsePacket makeRoomResponsePkt;

	if (newRoomPtr == nullptr)
	{
		makeRoomResponsePkt.isMade = false;
		PacketBufferRef responsePkt = jh_memory::MakeShared<PacketBuffer>(sizeof(makeRoomResponsePkt));

		*responsePkt << makeRoomResponsePkt;

		m_pOwner->SendPacket(sessionId, responsePkt);

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_NO_LOG, L"[HandleMakeRoomRequestPacket] Create room failed. SessionId : [0x%016llx], ServerRoom: [%hu]", sessionId, roomNum);

		return;
	}
	else
	{
		auto unicastFunc = [this](ULONGLONG sessionId, PacketBufferRef& _packet)
			{
				m_pOwner->SendPacket(sessionId, _packet);
			};

		newRoomPtr->SetUnicastFunc(unicastFunc);

		makeRoomResponsePkt.isMade = true;

		makeRoomResponsePkt.roomInfo.m_usCurUserCnt = newRoomPtr->GetCurUserCnt();
		makeRoomResponsePkt.roomInfo.m_usMaxUserCnt = newRoomPtr->GetMaxUserCnt();
		makeRoomResponsePkt.roomInfo.m_ullOwnerId = newRoomPtr->GetOwnerId();
		makeRoomResponsePkt.roomInfo.m_usRoomNum = newRoomPtr->GetRoomNum();
		wmemcpy_s(makeRoomResponsePkt.roomInfo.m_wszRoomName, ROOM_NAME_MAX_LEN, roomName, ROOM_NAME_MAX_LEN);

		PacketBufferRef responsePkt = jh_memory::MakeShared<PacketBuffer>(sizeof(makeRoomResponsePkt));

		*responsePkt << makeRoomResponsePkt;

		m_pOwner->SendPacket(sessionId, responsePkt);
	}

	newRoomPtr->TryEnterRoom(userPtr);

	{
		static std::vector<ULONGLONG> emptyVector;

		ULONGLONG userId = userPtr->GetUserId();

		PacketBufferRef enterRoomResponsePkt = jh_content::PacketBuilder::BuildEnterRoomResponsePacket(true, emptyVector);

		m_pOwner->SendPacket(sessionId, enterRoomResponsePkt);
	}
}

void jh_content::LobbySystem::HandleEnterRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	jh_network::EnterRoomRequestPacket requestPacket;

	*packet >> requestPacket;

	RoomPtr roomPtr = m_pRoomManager->GetRoom(requestPacket.roomNum);

	if (nullptr == roomPtr)
	{
		PacketBufferRef errorPkt = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::REQUEST_DESTROYED_ROOM);

		m_pOwner->SendPacket(sessionId, errorPkt);

		return;
	}

	if (wcscmp(requestPacket.roomName, static_cast<const WCHAR*>(roomPtr->GetRoomNamePtr())) != 0)
	{
		PacketBufferRef errorPkt = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::REQUEST_DIFF_ROOM_NAME);

		m_pOwner->SendPacket(sessionId, errorPkt);

		return;
	}

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (userPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleEnterRoomRequestPacket] User not found. SessionID: [0x%016llx]", sessionId);
		
		return;
	}

	auto [isAlreadyInRoom, curRoomNum] = userPtr->TryGetRoomId();

	if (true == isAlreadyInRoom)
	{
		m_invalidEnter++;

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleEnterRoomRequestPacket] User already in room. SessionId: [0x%016llx], ClientRoom: [%hu], ServerRoom: [%hu]", sessionId, requestPacket.roomNum, curRoomNum);

		return;
	}

	ULONGLONG userId = userPtr->GetUserId();

	jh_content::Room::RoomEnterResult res = roomPtr->TryEnterRoom(userPtr);

	switch (res)
	{
	case jh_content::Room::RoomEnterResult::SUCCESS:
	{	
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

		PacketBufferRef enterRoomResponsePkt = jh_content::PacketBuilder::BuildEnterRoomResponsePacket(true, userIdAndReadyList);

		m_pOwner->SendPacket(sessionId, enterRoomResponsePkt);

		{
			PacketBufferRef enterRoomNotifyPkt = jh_content::PacketBuilder::BuildEnterRoomNotifyPacket(userId);

			roomPtr->BroadCast(enterRoomNotifyPkt, userId);
		}

		break;
	}
	case jh_content::Room::RoomEnterResult::FULL:
	{
		PacketBufferRef errorPkt = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::FULL_ROOM);

		m_pOwner->SendPacket(sessionId, errorPkt);

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleEnterRoomRequestPacket] Room full. UserId : [%llu]", userId);

		break;
	}
	case jh_content::Room::RoomEnterResult::ALREADY_STARTED:
	{
		PacketBufferRef errorPkt = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::ALREADY_RUNNING_ROOM);

		m_pOwner->SendPacket(sessionId, errorPkt);

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleEnterRoomRequestPacket] Room already running. UserId : [%llu]", userId);

		break;
	}
	}
	return;
}
void jh_content::LobbySystem::HandleLeaveRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	jh_network::LeaveRoomRequestPacket leaveRoomRequestPacket;

	*packet >> leaveRoomRequestPacket;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (userPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleLeaveRoomRequestPacket] User not found. SessionId : [%llu]", sessionId);

		return;
	}
	auto [isAlreadyInRoom, roomNum] = userPtr->TryGetRoomId();

	if (false == isAlreadyInRoom || roomNum != leaveRoomRequestPacket.roomNum)
	{
		m_invalidLeave++;

		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L" [HandleLeaveRoomRequestPacket] - UserId : [%llu], ClientRoomNum : [%hu], ServerRoomNum : [%hu], IsAlreadyInRoom : [%s]",userPtr->GetUserId(), leaveRoomRequestPacket.roomNum,roomNum, isAlreadyInRoom ? "true" : "false");

		return;
	}

	RoomPtr roomPtr = m_pRoomManager->GetRoom(leaveRoomRequestPacket.roomNum);

	if (roomPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleLeaveRoomRequestPacket] Room not found. Room: [%hu], SessionId : [0x%016llx], UserId : [%llu] ", leaveRoomRequestPacket.roomNum, sessionId,userPtr->GetUserId());

		return;
	}

	if (wcscmp(leaveRoomRequestPacket.roomName, static_cast<const WCHAR*>(roomPtr->GetRoomNamePtr())) != 0)
		return;


	PacketBufferRef leaveRoomResPkt = jh_content::PacketBuilder::BuildLeaveRoomResponsePacket();
	m_pOwner->SendPacket(sessionId, leaveRoomResPkt);

	bool isRoomEmpty = roomPtr->LeaveRoom(userPtr);
	
	if (isRoomEmpty == true)
		m_pRoomManager->DestroyRoom(roomPtr->GetRoomNum());
}

void jh_content::LobbySystem::HandleGameReadyRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	jh_network::GameReadyRequestPacket requestPacket;

	*packet >> requestPacket.isReady;

	UserPtr userPtr = m_pUserManager->GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameReadyRequest] User not found. SessionID: [0x%016llx]", sessionId);
		
		return;
	}

	auto [res, roomNum] = userPtr->TryGetRoomId();

	if (false == res)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GameReadyRequest] User not in room. UserId: [%llu]", userPtr->GetUserId());

		m_invalidReady++;
		
		return;
	}
	
	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	bool canStart = roomPtr->UpdateUserReadyStatus(userPtr, requestPacket.isReady, true);

	if (true == canStart)
	{
		std::wstring path(GAME_FILE_PATH);

		ExecuteProcess(GAME_FILE_PATH, GAME_CUR_DIRECTORY);

		USHORT gameRoomNum = roomPtr->GetRoomNum();
		
		m_pendingGameRoomList.emplace_back(gameRoomNum);
	}
}

void jh_content::LobbySystem::HandleEchoPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	ULONGLONG data;

	*packet >> data;

	PacketBufferRef echoPkt = jh_content::PacketBuilder::BuildEchoPacket(data);
	
	m_pOwner->SendPacket(sessionId, echoPkt);

	return;
}

void jh_content::LobbySystem::HandleHeartbeatPacket(ULONGLONG sessionId, PacketBufferRef& packet)
{
	PRO_START_AUTO_FUNC;

	ULONGLONG packetTimeStamp;

	*packet >> packetTimeStamp;

	m_pOwner->UpdateHeartbeat(sessionId, packetTimeStamp);

	return;
}

void jh_content::LobbySystem::HandleLanInfoNotify(ULONGLONG lanSessionId, PacketBufferRef& lanPacket, jh_network::IocpServer* lanServer)
{
	PRO_START_AUTO_FUNC;

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

	PacketBufferRef sendBuffer = jh_content::PacketBuilder::BuildLanInfoPacket(ipStr, port, roomNum, xorToken ^ xorTokenKey);// C_Network::BufferMaker::MakeSendBuffer(sizeof(lanInfoPacket));

	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	if (roomPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleLanInfoNotify] Room not found: [%hu]", roomNum);

		return;
	}

	roomPtr->BroadCast(sendBuffer);
}

void jh_content::LobbySystem::HandleGameSettingRequest(ULONGLONG lanSessionId, PacketBufferRef& lanPacket, jh_network::IocpServer* lanServer)
{
	PRO_START_AUTO_FUNC;

	_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[HandleGameSettingRequest] GameSetting RequestPacket Recv");

	printf("Game Setting Request Packet Recv\n");

	if (m_pendingGameRoomList.empty())
		return;

	USHORT roomNum = m_pendingGameRoomList.back();
	m_pendingGameRoomList.pop_back();

	RoomPtr roomPtr = m_pRoomManager->GetRoom(roomNum);

	if (roomPtr == nullptr)
	{
		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[HandleGameSettingRequest] Room not found: [%hu]", roomNum);
		return;
	}

	USHORT requiredUserCnt = roomPtr->GetCurUserCnt();
	USHORT maxUserCnt = roomPtr->GetMaxUserCnt();

	PacketBufferRef gameServerSettingResponsePkt = jh_content::PacketBuilder::BuildGameServerSettingResponsePacket(roomNum, requiredUserCnt, maxUserCnt);

	lanServer->SendPacket(lanSessionId, gameServerSettingResponsePkt);
}
