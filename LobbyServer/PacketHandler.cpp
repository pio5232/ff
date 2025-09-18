//#include "pch.h"
//#include "LobbyServer.h"
////#include "LobbySession.h"
//#include "PacketHandler.h"
//#include "BufferMaker.h"
//#include "RoomManager.h"
//#include "UserManager.h"
//#include <process.h>
//#include "User.h"
//
//using namespace jh_content;
///*---------------------------------------
//			LobbyClientPacketHandler
//---------------------------------------*/
//
////std::unordered_map<uint16, jh_network::LobbyClientPacketHandler::PacketFunc> jh_network::LobbyClientPacketHandler::m_packetFuncsDic;
////std::unordered_map<uint16, jh_network::LanClientPacketHandler::PacketFunc> jh_network::LanClientPacketHandler::m_packetFuncsDic;
////
////void jh_network::LobbyClientPacketHandler::Init()
////{
////	m_packetFuncsDic.clear();
////
////	m_packetFuncsDic[ROOM_LIST_REQUEST_PACKET] = ProcessRoomListRequestPacket;
////
////	m_packetFuncsDic[CHAT_TO_ROOM_REQUEST_PACKET] = ProcessChatToRoomPacket;
////	m_packetFuncsDic[CHAT_TO_USER_REQUEST_PACKET] = ProcessChatToUserPacket;
////
////	m_packetFuncsDic[LOG_IN_REQUEST_PACKET] = ProcessLogInPacket;
////	m_packetFuncsDic[MAKE_ROOM_REQUEST_PACKET] = ProcessMakeRoomRequestPacket;
////
////	m_packetFuncsDic[ENTER_ROOM_REQUEST_PACKET] = ProcessEnterRoomRequestPacket;
////	m_packetFuncsDic[LEAVE_ROOM_REQUEST_PACKET] = ProcessLeaveRoomRequestPacket;
////
////	m_packetFuncsDic[GAME_READY_REQUEST_PACKET] = ProcessGameReadyRequestPacket;
////
////	m_packetFuncsDic[HEART_BEAT_PACKET] = ProcessHeartbeatPacket;
////
////}
////
////
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessChatToRoomPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	wprintf(L" ChatToRoom Request Recv");
////
////	uint16 roomNum;
////	uint16 messageLen;
////
////	buffer >> roomNum >> messageLen;
////
////	char* payLoad = static_cast<char*>(malloc(messageLen));
////
////	buffer.GetData(payLoad, messageLen);
////
////	PacketHeader packetHeader;
////
////	packetHeader.size = 0;
////	packetHeader.type = CHAT_TO_ROOM_RESPONSE_PACKET;
////
////	// --- ChatRoomResponsePacket
////	jh_network::SharedSendBuffer responseBuffer = jh_network::BufferMaker::MakePacket(packetHeader);
////	lobbySessionPtr->Send(responseBuffer);
////
////	ULONGLONG userId = lobbySessionPtr->_userId;
////
////	// --- NotifyPacket
////	packetHeader.size = sizeof(userId) + sizeof(messageLen) + messageLen;
////	packetHeader.type = CHAT_NOTIFY_PACKET;
////
////	jh_network::SharedSendBuffer notifyBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(packetHeader) + packetHeader.size);
////
////	*notifyBuffer << packetHeader << userId << messageLen;
////	notifyBuffer->PutData(reinterpret_cast<const char*>(payLoad), messageLen);
////
////	free(payLoad);
////
////	RoomPtr roomPtr = RoomManager::GetInstance().GetRoom(roomNum);
////
////	if (roomPtr == nullptr)
////	{
////		// LOG
////		return ErrorCode::ACCESS_DESTROYED_ROOM;
////	}
////	//roomPtr->DoAsync(&Room::ChatRoom, userId, notifyBuffer);
////	roomPtr->DoAsync(&Room::SendToAll, notifyBuffer, userId, false);
////
////	return ErrorCode::NONE;
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessChatToUserPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////
////	return ErrorCode::NONE;
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessMakeRoomRequestPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	wprintf(L" MakeRoom Request Recv");
////
////	WCHAR roomName[ROOM_NAME_MAX_LEN]{};
////
////	buffer.GetData(reinterpret_cast<char*>(&roomName), sizeof(roomName));
////
////	uint16 userId = lobbySessionPtr->_userId;
////
////	RoomPtr roomPtr = RoomManager::GetInstance().CreateRoom(lobbySessionPtr, roomName);
////
////	ErrorCode ret = ErrorCode::NONE;
////
////	jh_network::MakeRoomResponsePacket makeRoomResponsePacket;
////
////	if (roomPtr == nullptr)
////	{
////		makeRoomResponsePacket.isMade = false;
////
////		printf("EnterRoom Response Packet Send -- [bAllow = false]");
////
////		ret = ErrorCode::CREATE_ROOM_FAILED;
////	}
////	else
////	{
////		makeRoomResponsePacket.isMade = true;
////
////		makeRoomResponsePacket.roomInfo.curUserCnt = roomPtr->GetCurUserCnt();
////		makeRoomResponsePacket.roomInfo.maxUserCnt = roomPtr->GetMaxUserCnt();
////		makeRoomResponsePacket.roomInfo.ownerId = roomPtr->GetOwnerId();
////		makeRoomResponsePacket.roomInfo.roomNum = roomPtr->GetRoomNum();
////		wmemcpy_s(makeRoomResponsePacket.roomInfo.roomName, ROOM_NAME_MAX_LEN, roomName, ROOM_NAME_MAX_LEN);
////	}
////
////	jh_network::SharedSendBuffer responsePacketBuffer = jh_network::BufferMaker::MakePacket(makeRoomResponsePacket);
////
////	lobbySessionPtr->Send(responsePacketBuffer);
////
////	return ret;
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessEnterRoomRequestPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	jh_network::EnterRoomRequestPacket requestPacket;
////
////	buffer >> requestPacket;
////
////	RoomPtr roomPtr = RoomManager::GetInstance().GetRoom(requestPacket.roomNum);
////
////	if (nullptr == roomPtr)
////	{
////		SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeErrorPacket(PacketErrorCode::REQUEST_DESTROYED_ROOM);
////
////		lobbySessionPtr->Send(sendBuffer);
////		printf("Invalid Access - Destroyed Room");
////		return ErrorCode::CANNOT_FIND_ROOM;
////	}
////
////	if (wcscmp(requestPacket.roomName, static_cast<const WCHAR*>(roomPtr->GetRoomNamePtr())) != 0)
////	{
////		SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeErrorPacket(PacketErrorCode::REQUEST_DIFF_ROOM_NAME);
////
////		lobbySessionPtr->Send(sendBuffer);
////		printf("EnterRoom - Room name is Diffrent\n");
////		return ErrorCode::CANNOT_FIND_ROOM;
////	}
////
////	ULONGLONG userId = lobbySessionPtr->_userId;
////
////	printf("EnterRoom Response Packet Send  ");
////
////	roomPtr->DoAsync(&Room::EnterRoom, lobbySessionPtr);
////
////	return ErrorCode::NONE;
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessLeaveRoomRequestPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	jh_network::LeaveRoomRequestPacket leaveRoomRequestPacket;
////
////	buffer >> leaveRoomRequestPacket;
////
////	RoomPtr roomPtr = RoomManager::GetInstance().GetRoom(leaveRoomRequestPacket.roomNum);
////
////	if (roomPtr == nullptr)
////	{
////		// LOG
////		return ErrorCode::ACCESS_DESTROYED_ROOM;
////	}
////
////	if (wcscmp(leaveRoomRequestPacket.roomName, static_cast<const WCHAR*>(roomPtr->GetRoomNamePtr())) != 0)
////	{
////		printf("LeaveRoom - Room name is Diffrent\n");
////		return ErrorCode::CANNOT_FIND_ROOM;
////	}
////
////	ULONGLONG userId = lobbySessionPtr->_userId;
////
////	roomPtr->DoAsync(&Room::LeaveRoom, lobbySessionPtr);
////
////	return ErrorCode();
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessGameReadyRequestPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	jh_network::GameReadyRequestPacket requestPacket;
////
////	buffer >> requestPacket.isReady;
////
////	ULONGLONG userId = lobbySessionPtr->_userId;
////
////	std::weak_ptr<Room> room = lobbySessionPtr->GetRoom();
////	if (room.expired())
////	{
////		wprintf(L"ProcessGameReadyRequestPacket - Room Not Found");
////		return ErrorCode::CANNOT_FIND_ROOM;
////	}
////
////	RoomPtr sharedRoom = room.lock();
////
////	sharedRoom->DoAsync(&Room::SetReady, lobbySessionPtr, requestPacket.isReady, true);
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessHeartbeatPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	ULONGLONG packetTimeStamp;
////
////	buffer >> packetTimeStamp;
////	printf("SessionId : %llu, [comTimeStamp : %llu], [PacketTimeStamp : %llu]\n", lobbySessionPtr->GetSessionId(), jh_utility::GetTimeStamp(), packetTimeStamp);
////	
////	lobbySessionPtr->UpdateHeartbeat(packetTimeStamp);
////
////	return ErrorCode::NONE;
////}
////
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessRoomListRequestPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	std::cout << "RoomList Request\n";
////
////	ErrorCode errCode = RoomManager::GetInstance().SendToUserRoomInfo(lobbySessionPtr);
////
////	return errCode;
////}
////
////ErrorCode jh_network::LobbyClientPacketHandler::ProcessLogInPacket(LobbySessionPtr& lobbySessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	std::cout << "LogIn Request\n";
////
////	LogInRequestPacket clientRequestPacket;
////
////	buffer >> clientRequestPacket;
////	// 접속되어있는 녀석이 로그인을 누르면 무시.
////	// 
////	// ID와 비밀번호를 확인한 후 (클라이언트에서 암호화 -> 서버에서 복호화?)
////	// 검증
////
////	// 원래는 DB에서 USER 정보를 얻어와야 하지만, 현재는 DB 적용하지 않았기 때문에 3씩 증가시키도록 한다.
////	static volatile ULONGLONG userIdGenerator = 4283;
////
////	// AAA : DB에서 얻어오는걸로 변경.
////	ULONGLONG userId = InterlockedAdd64((LONGLONG*)&userIdGenerator, 3);
////
////	LogInResponsePacket clientResponsePacket;
////
////	clientResponsePacket.size = sizeof(clientResponsePacket.userId);
////	clientResponsePacket.type = LOG_IN_RESPONSE_PACKET;
////	clientResponsePacket.userId = userId;
////
////	lobbySessionPtr->_userId = userId;
////
////	jh_network::SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakePacket(clientResponsePacket);
////
////	lobbySessionPtr->Send(sendBuffer);
////
////	// 로그인 데이터를 DB에서 불러와서 설정 후 로그인 정보를
////
////	// 해당 클라이언트에 전송한다...?
////
////	// 현재는 일단 userId를 전송하도록 한다.
////
////	return ErrorCode::NONE;
////}
////
////
////void jh_network::LanClientPacketHandler::Init()
////{
////	m_packetFuncsDic.clear();
////
////	m_packetFuncsDic[GAME_SERVER_LAN_INFO_PACKET] = ProcessLanInfoNotifyPacket; // ip Port
////	m_packetFuncsDic[GAME_SERVER_SETTING_REQUEST_PACKET] = ProcessGameSettingRequestPacket; // completePacket
////
////}
////
////ErrorCode jh_network::LanClientPacketHandler::ProcessLanInfoNotifyPacket(LanSessionPtr& lanSessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	printf("Lan Info Notify Packet Recv\n");
////	jh_network::GameServerLanInfoPacket lanInfoPacket;
////	
////	buffer.GetData(reinterpret_cast<char*>(&lanInfoPacket.ipStr[0]), sizeof(lanInfoPacket.ipStr));
////
////	buffer >> lanInfoPacket.port >> lanInfoPacket.roomNum >> lanInfoPacket.xorToken;
////
////	wprintf(L"size : [ %d ]\n", lanInfoPacket.size);
////	wprintf(L"type : [ %d ]\n", lanInfoPacket.type);
////	wprintf(L"ip: [ %s ]\n", lanInfoPacket.ipStr);
////	wprintf(L"port : [ %d ]\n", lanInfoPacket.port);
////	wprintf(L"Room : [ %d ]\n", lanInfoPacket.roomNum);
////	wprintf(L"xorToken : [ %llu ], After : [%llu]\n", lanInfoPacket.xorToken, lanInfoPacket.xorToken ^ xorTokenKey);
////
////	jh_network::SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(lanInfoPacket));
////
////	*sendBuffer << lanInfoPacket.size << lanInfoPacket.type;
////
////	sendBuffer->PutData(reinterpret_cast<const char*>(lanInfoPacket.ipStr), IP_STRING_LEN * sizeof(WCHAR));
////
////	*sendBuffer << lanInfoPacket.port << lanInfoPacket.roomNum << lanInfoPacket.xorToken;
////
////	RoomPtr roomPtr = jh_network::RoomManager::GetInstance().GetRoom(lanInfoPacket.roomNum);
////
////	if (roomPtr == nullptr)
////	{
////		// LOG
////		return ErrorCode::ACCESS_DESTROYED_ROOM;
////	}
////
////	roomPtr->SendToAll(sendBuffer);
////
////	return ErrorCode::NONE;
////}
////
////ErrorCode jh_network::LanClientPacketHandler::ProcessGameSettingRequestPacket(LanSessionPtr& lanSessionPtr, jh_utility::SerializationBuffer& buffer)
////{
////	printf("Game Setting Request Packet Recv\n");
////	uint16 roomNum = jh_network::RoomManager::GetInstance().PopRoomNum();
////	
////	RoomPtr roomPtr = jh_network::RoomManager::GetInstance().GetRoom(roomNum);
////
////	if (roomPtr == nullptr)
////	{
////		// LOG
////		printf("GameSetting.. Room is Invalid\n");
////		return ErrorCode::ACCESS_DESTROYED_ROOM;
////	}
////
////	jh_network::GameServerSettingResponsePacket settingResponsePacket;
////
////	settingResponsePacket.roomNum = roomNum;
////	settingResponsePacket.requiredUsers = roomPtr->GetCurUserCnt();
////	settingResponsePacket.m_usMaxUserCnt = roomPtr->GetMaxUserCnt();
////	
////	SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(settingResponsePacket));
////
////	*sendBuffer << settingResponsePacket.size << settingResponsePacket.type << settingResponsePacket.roomNum << settingResponsePacket.requiredUsers << settingResponsePacket.m_usMaxUserCnt;
////	
////	lanSessionPtr->Send(sendBuffer);
////
////	return ErrorCode::NONE;
////}
//
////jh_content::LobbySystem::LobbySystem(jh_network::IocpServer* owner) :m_pOwner(owner), m_bRunningFlag(true)
////{
////	if (nullptr == m_pOwner)
////	{
////		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"LobbySystem() - Owner (Server) is NULL");
////		jh_utility::CrashDump::Crash();
////	}
////	m_pRoomManager = std::unique_ptr<jh_content::RoomManager>();
////
////	m_pUserManager = std::unique_ptr<jh_content::UserManager>();
////}
//
//jh_content::LobbySystem::~LobbySystem()
//{
//}
//
//
//JobPtr jh_content::LobbySystem::DequeueJob()
//{
//	return JobPtr();
//}
//
///// <summary>
/////  -- Modified LoobyPacketHandler
///// </summary>
//void jh_content::LobbySystem::Init()
//{
//	m_pRoomManager->Init();
//	m_pUserManager->Init();
//
//	m_packetFuncsDic.clear();
//
//	m_packetFuncsDic[jh_network::ROOM_LIST_REQUEST_PACKET] = LobbySystem::ProcessRoomListRequestPacket;
//
//	m_packetFuncsDic[jh_network::CHAT_TO_ROOM_REQUEST_PACKET] = LobbySystem::ProcessChatToRoomPacket;
//	m_packetFuncsDic[jh_network::CHAT_TO_USER_REQUEST_PACKET] = LobbySystem::ProcessChatToUserPacket;
//
//	m_packetFuncsDic[jh_network::LOG_IN_REQUEST_PACKET] = LobbySystem::ProcessLogInPacket;
//	m_packetFuncsDic[jh_network::MAKE_ROOM_REQUEST_PACKET] = LobbySystem::ProcessMakeRoomRequestPacket;
//
//	m_packetFuncsDic[jh_network::ENTER_ROOM_REQUEST_PACKET] = LobbySystem::ProcessEnterRoomRequestPacket;
//	m_packetFuncsDic[jh_network::LEAVE_ROOM_REQUEST_PACKET] = LobbySystem::ProcessLeaveRoomRequestPacket;
//
//	m_packetFuncsDic[jh_network::GAME_READY_REQUEST_PACKET] = LobbySystem::ProcessGameReadyRequestPacket;
//
//	m_packetFuncsDic[jh_network::HEART_BEAT_PACKET] = LobbySystem::ProcessHeartbeatPacket;
//
//	LPVOID param = this;
//	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LobbySystem::StaticLogicProxy, param, 0, nullptr));
//
//	if (NULL == m_hLogicThread)
//	{
//		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"Lobby System - Logic Handle is NULL");
//		jh_utility::CrashDump::Crash();
//	}
//}
//
//unsigned __stdcall jh_content::LobbySystem::StaticLogicProxy(LPVOID lparam)
//{
//	jh_content::LobbySystem* lobbyInstance = static_cast<LobbySystem*>(lparam);
//
//	lobbyInstance->LobbyLogic();
//}
//
//void jh_content::LobbySystem::LobbyLogic()
//{
//	while (true == m_bRunningFlag)
//	{
//		// Packet 관련 작업 처리
//		ProcessNetJob();
//
//		// System 관련 작업 처리.
//		ProcessSessionConnectionEvent();
//
//		// 채팅이기 때문에 들어오는대로 바로 작업을 처리해도 괜찮아보임.
//	}
//}
//
//void jh_content::LobbySystem::Stop()
//{
//	m_bRunningFlag = false;
//
//	DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);
//
//	if (ret != WAIT_OBJECT_0)
//	{
//		DWORD error = GetLastError();
//		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"Lobby System Stop - WaitForSingleObject : %u, GetLastError : %u", ret, error);
//		jh_utility::CrashDump::Crash();
//		return;
//	}
//
//	_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"Logic Thread Shutdown");
//
//}
//
//void jh_content::LobbySystem::ProcessNetJob()
//{
//	static std::vector<JobPtr> jobList(10);
//	jobList.clear();
//
//	_netJobQueue.PopAll(jobList);
//
//	for (const JobPtr& job : jobList)
//	{
//		ProcessPacket(job->sessionPtr, job->jobType, job->serialBufferPtr);
//	}
//	// netJobQueue에서 Job 확인 후 
//	// 작업따라 패킷 처리
//
//}
//
//void jh_content::LobbySystem::ProcessSessionConnectionEvent()
//{
//	static std::vector<SessionConnectionEventPtr> systemJobList(10);
//	systemJobList.clear();
//
//	_systemJobQueue.PopAll(systemJobList);
//
//	for (const SessionConnectionEventPtr& job : systemJobList)
//	{
//		switch (job->msgType)
//		{
//		case jh_utility::SessionConnectionEventType::CONNECT:
//		{
//			// 유저 접속 처리.
//		}
//		break;
//
//		case jh_utility::SessionConnectionEventType::DISCONNECT: break;
//		{
//			// 유저 접속 종료 처리.
//		}
//		break;
//		}
//	}
//	// systemJobQueue에서 Job 확인 후 
//	// 작업따라 패킷 처리.
//}
//
//// Update V.
//ErrorCode jh_content::LobbySystem::ProcessMakeRoomRequestPacket(SessionPtr& sessionPtr, jh_network::SerializationBufferPtr& serializationBufferPtr)
//{
//	WCHAR roomName[ROOM_NAME_MAX_LEN]{};
//
//	serializationBufferPtr->GetData(reinterpret_cast<char*>(&roomName), sizeof(roomName));
//
//	ULONGLONG sessionId = sessionPtr->sessionId;
//	UserPtr userPtr = m_pUserManager->GetUser(sessionPtr->sessionId);
//
//	if (nullptr == userPtr)
//	{
//		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"ProcMakeRoomRequestPacket - User is not exist, SessionId : %llu", sessionId);
//		return;
//	}
//
//	RoomPtr roomPtr = m_pRoomManager->CreateRoom(userPtr, roomName);
//
//
//	ULONGLONG ownerId = roomPtr->GetOwnerId();
//	uint16 roomNum = roomPtr->GetRoomNum();
//	uint16 curUserCnt = roomPtr->GetCurUserCnt();
//	uint16 maxUserCnt = roomPtr->GetMaxUserCnt();
//
//	jh_network::SerializationBufferPtr responsePacketBuffer = jh_content::PacketBuilder::BuildMakeRoomResponsePacket(true, ownerId, roomNum, curUserCnt, maxUserCnt, roomName);
//	m_pOwner->Send(sessionPtr, responsePacketBuffer);
//
//	jh_content::Room::RoomEnterResult tryEnterResult = roomPtr->TryEnterRoom(userPtr);
//
//	switch (tryEnterResult)
//	{
//	case jh_content::Room::RoomEnterResult::SUCCESS:
//	{
//		jh_network::SerializationBufferPtr roomInfoBuffer = roomPtr->MakeRoomInfo();
//		m_pOwner->Send(sessionPtr, roomInfoBuffer);
//
//		jh_network::SerializationBufferPtr enterUserInfoBuffer = PacketBuilder::BuildEnterRoomNotifyPacket(userPtr->GetUserId());
//		// m_pOwner->SendRoom과 같이 Room에 있는 모든 유저에게 보내야하는데.
//
//		break;
//	}
//	case jh_content::Room::RoomEnterResult::FULL:
//	{
//		jh_network::SerializationBufferPtr sendBuffer = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::FULL_ROOM);
//
//		m_pOwner->Send(sessionPtr, sendBuffer);
//
//		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"ProcessMakeRoomRequestPacket - Create->TryEnter Failed, Full");
//		return;
//	}
//	case jh_content::Room::RoomEnterResult::ALREADY_STARTED:
//	{
//		jh_network::SerializationBufferPtr sendBuffer = jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode::ALREADY_RUNNING_ROOM);
//
//		m_pOwner->Send(sessionPtr, sendBuffer);
//
//		_LOG(LOBBY_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"ProcessMakeRoomRequestPacket - Create->TryEnter Failed, Already Started");
//		return;
//	}
//	}
//
//	return ErrorCode::NONE;
//}
