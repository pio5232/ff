#include "pch.h"
#include "DummyUpdateSystem.h"
#include "DummyPacketBuilder.h"
#include "Memory.h"
unsigned jh_content::DummyUpdateSystem::LogicThreadFunc(LPVOID lparam)
{
	WorkerTransData* data = static_cast<WorkerTransData*>(lparam);
	
	srand(data->m_iThreadNum);

	DummyUpdateSystem* dummyInstance = static_cast<DummyUpdateSystem*>(data->m_pThis);
	
	if (nullptr == dummyInstance)
		return 0;

	dummyInstance->DummyLogic(data->m_iThreadNum);

	return 0;
}

void jh_content::DummyUpdateSystem::DummyLogic(int threadNum)
{
	_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[DummyLogic] - threadNum : [%d], ThreadID : [%u]", threadNum, GetCurrentThreadId());

	const HANDLE jobEvent = m_logicData[threadNum].m_hJobEvent;
	
	static thread_local ULONGLONG lastSendCheckTime = 0;

	while (true == m_bRunnigFlag)
	{
		WaitForSingleObject(jobEvent, 10);

		PRO_START("DummyLogic");
		ProcessNetJob(threadNum);

		ProcessSessionConnectionEvent(threadNum);

		ProcessDummyLogic(threadNum);

		ULONGLONG curTime = jh_utility::GetTimeStamp();

		// 1초에 한 번 체크
		if (curTime - lastSendCheckTime > 2000)
		{
			lastSendCheckTime = curTime;
			
			CheckSendTimeOut(threadNum);
		}
		PRO_END("DummyLogic");
	}

	
}


jh_content::DummyUpdateSystem::DummyUpdateSystem(jh_network::IocpClient* owner) : m_pOwner{ owner }, m_bRunnigFlag{ 1 }, m_ullRtt{}
{

}

jh_content::DummyUpdateSystem::~DummyUpdateSystem()
{
	if (0 != m_bRunnigFlag)
	{
		Stop();
	}
}

void jh_content::DummyUpdateSystem::Init()
{
	m_packetFuncDic.clear();

	m_packetFuncDic[jh_network::ROOM_LIST_RESPONSE_PACKET] = &DummyUpdateSystem::HandleRoomListResponsePacket;
	m_packetFuncDic[jh_network::CHAT_NOTIFY_PACKET] = &DummyUpdateSystem::HandleChatNotifyPacket;
	m_packetFuncDic[jh_network::CHAT_TO_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleChatResponsePacket;
	m_packetFuncDic[jh_network::LOG_IN_RESPONSE_PACKET] = &DummyUpdateSystem::HandleLogInResponsePacket;
	m_packetFuncDic[jh_network::MAKE_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleMakeRoomResponsePacket;
	m_packetFuncDic[jh_network::ENTER_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleEnterRoomResponsePacket;
	m_packetFuncDic[jh_network::LEAVE_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleLeaveRoomResponsePacket;
	m_packetFuncDic[jh_network::ERROR_PACKET] = &DummyUpdateSystem::HandleErrorPacket;
	m_packetFuncDic[jh_network::ECHO_PACKET] = &DummyUpdateSystem::HandleEchoPacket;

	WorkerTransData transData[LOGIC_THREAD_COUNT];


	for (int i = 0; i < LOGIC_THREAD_COUNT; i++)
	{
		LogicData& logicData = m_logicData[i];

		logicData.m_hJobEvent = CreateEvent(nullptr, false, false, nullptr);
		if (nullptr == logicData.m_hJobEvent)
		{
			DWORD lastError = GetLastError();
			_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[DummyUpdateSystem] - JobEvent 핸들이 존재하지 않습니다. 에러 코드 : [%u], 인덱스 : [%d]", lastError,i);

			jh_utility::CrashDump::Crash();
		}

		transData[i].m_pThis = this;
		transData[i].m_iThreadNum = i;

		logicData.m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, DummyUpdateSystem::LogicThreadFunc, &transData[i], 0, nullptr));

		if (nullptr == logicData.m_hLogicThread)
		{
			_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[DummyUpdateSystem] - 로직 핸들이 존재하지 않습니다. 인덱스 : [%d]",i);

			jh_utility::CrashDump::Crash();
		}
	}
	Sleep(1000);
}

void jh_content::DummyUpdateSystem::Stop()
{
	InterlockedExchange8(&m_bRunnigFlag, 0);
	
	for (int i = 0; i < LOGIC_THREAD_COUNT; i++)
	{
		SetEvent(m_logicData[i].m_hJobEvent);
	}

	for (int i = 0; i < LOGIC_THREAD_COUNT; i++)
	{
		LogicData& logicData = m_logicData[i];

		if (nullptr != logicData.m_hLogicThread)
		{
			DWORD ret = WaitForSingleObject(logicData.m_hLogicThread, INFINITE);

			if (ret != WAIT_OBJECT_0)
			{
				DWORD getLastError = GetLastError();

				_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[DummyUpdateSystem - Stop] 로직 스레드 종료 오류 GetLastError : [%u], idx : [%d]", getLastError, i);
			}
			else
			{
				CloseHandle(logicData.m_hLogicThread);
				CloseHandle(logicData.m_hJobEvent);

				logicData.m_hLogicThread = nullptr;
				logicData.m_hJobEvent = nullptr;
				logicData.m_netJobQueue.Clear();
			}
		}
	}

	_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_INFO, L"[DummyUpdateSystem - Stop] 로직 스레드 정상 종료");
}

void jh_content::DummyUpdateSystem::EnqueueJob(JobPtr& job, int idx)
{
	if (idx < 0 || idx >= LOGIC_THREAD_COUNT)
		return;

	LogicData& logicData = m_logicData[idx];

	logicData.m_netJobQueue.Push(job);

	SetEvent(logicData.m_hJobEvent);
}

void jh_content::DummyUpdateSystem::EnqueueSessionConnEvent(SessionConnectionEventPtr& connectionEvent, int idx)
{
	if (idx < 0 || idx >= LOGIC_THREAD_COUNT)
		return;

	LogicData& logicData = m_logicData[idx];

	logicData.m_sessionConnEventQueue.Push(connectionEvent);

	SetEvent(logicData.m_hJobEvent);
}

void jh_content::DummyUpdateSystem::ProcessNetJob(int threadNum)
{
	PRO_START_AUTO_FUNC;

	static thread_local alignas(64) std::queue<JobPtr> dummyJobQ;

	std::queue<JobPtr> emptyQ;

	m_logicData[threadNum].m_netJobQueue.Swap(dummyJobQ);

	while(dummyJobQ.size() > 0)
	{
		JobPtr& job = dummyJobQ.front();
	
		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket, threadNum);
	
		dummyJobQ.pop();
	}
	
	dummyJobQ.swap(emptyQ);
}

void jh_content::DummyUpdateSystem::ProcessSessionConnectionEvent(int threadNum)
{
	PRO_START_AUTO_FUNC;

	LogicData& threadLogicData = m_logicData[threadNum];

	static thread_local alignas(64) std::queue<SessionConnectionEventPtr> sessionConnEventQ;
	std::queue<SessionConnectionEventPtr> emptyQ;

	threadLogicData.m_sessionConnEventQueue.Swap(sessionConnEventQ);
	
	while(sessionConnEventQ.size() > 0)
	{
		SessionConnectionEventPtr& sessionConnEvent = sessionConnEventQ.front();
		ULONGLONG sessionId = sessionConnEvent->m_ullSessionId;

		switch (sessionConnEvent->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{			
			DummyPtr dummy = MakeShared<DummyData>(g_memSystem);
			
			_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_INFO, L"[ProcessSessionConnectionEvent] SessionID : [0x%08x]", sessionId);;

			dummy->m_dummyStatus = DummyStatus::SESSION_CONNECTED;
			dummy->m_ullSessionId = sessionId;
			dummy->m_ullLastUpdatedHeartbeatTime = jh_utility::GetTimeStamp();

			threadLogicData.m_dummyUmap[sessionId] = dummy;
			threadLogicData.m_dummyVector.push_back(dummy);
			
			int idx = static_cast<int>(threadLogicData.m_dummyVector.size() - 1);
			
			threadLogicData.m_dummyVectorIndexUMap[dummy] = idx;

			// 등록
			PacketPtr hbPkt = jh_content::DummyPacketBuilder::BuildHeartbeatPacket(dummy->m_ullLastUpdatedHeartbeatTime);

			PacketPtr logInReqPkt = jh_content::DummyPacketBuilder::BuildLoginRequestPacket();

			m_pOwner->SendPacket(dummy->m_ullSessionId, hbPkt);

			SendToDummy(dummy, logInReqPkt);
		}
		break;
		case jh_utility::SessionConnectionEventType::DISCONNECT:
		{
			if (threadLogicData.m_dummyUmap.find(sessionId) == threadLogicData.m_dummyUmap.end())
				break;

			DummyPtr dummy = threadLogicData.m_dummyUmap[sessionId];

			threadLogicData.m_dummyUmap.erase(sessionId);

			// vector idx 얻어오기
			int idx = threadLogicData.m_dummyVectorIndexUMap[dummy];
		
			int lastIdx = threadLogicData.m_dummyVector.size() - 1;
			
			threadLogicData.m_dummyVectorIndexUMap.erase(dummy);
			
			if (lastIdx != idx)
			{
				std::swap(threadLogicData.m_dummyVector[idx], threadLogicData.m_dummyVector[lastIdx]);

				threadLogicData.m_dummyVectorIndexUMap[threadLogicData.m_dummyVector[idx]] = idx;
			}

			threadLogicData.m_dummyVector.pop_back();
		}
		break;
		default:break;
		}

		sessionConnEventQ.pop();
	}

	sessionConnEventQ.swap(emptyQ);
}

void jh_content::DummyUpdateSystem::ProcessDummyLogic(int threadNum)
{
	PRO_START_AUTO_FUNC;

	std::vector<DummyPtr>& dummyVec = m_logicData[threadNum].m_dummyVector;

	ULONGLONG curTimeStamp = jh_utility::GetTimeStamp();

	for (DummyPtr& dummy : dummyVec)
	{

		if (curTimeStamp - dummy->m_ullLastUpdatedHeartbeatTime > 5000)
		{
			dummy->m_ullLastUpdatedHeartbeatTime = curTimeStamp;

			PacketPtr hbPkt = jh_content::DummyPacketBuilder::BuildHeartbeatPacket(curTimeStamp);
			
			m_pOwner->SendPacket(dummy->m_ullSessionId, hbPkt);
		}

		if (0 == clientSendFlag)
			continue;

		if (dummy->m_ullNextActionTime > curTimeStamp)
			continue;

		switch (dummy->m_dummyStatus)
		{
		case DummyStatus::IN_LOBBY:
		{
			PacketPtr roomListRqPkt = DummyPacketBuilder::BuildRoomListRequestPacket();

			SendToDummy(dummy, roomListRqPkt);

			break;
		}
		case DummyStatus::IN_ROOM:
		{
			int ran = GetRandValue(100,0);

			if (ran < 15)
			{
				PacketPtr leaveRoomRqPkt = DummyPacketBuilder::BuildLeaveRoomRequestPacket(dummy->m_usExpectedRoomNum, dummy->m_wszExpectedRoomName);

				SendToDummy(dummy, leaveRoomRqPkt, DummyStatus::WAIT_LEAVE_ROOM);

				//_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[ProcessDummyLogic] - INDEX : [%d], THREAD : [%u], Send LeavePacket, UserId : [%llu], RoomNum : [%hu], ActionTime : [%llu], curTime : [%llu]",threadNum, GetCurrentThreadId(), dummy->m_ullUserId, dummy->m_usExpectedRoomNum,dummy->m_ullNextActionTime, curTimeStamp);			
			}
			else
			{
				PacketPtr chatPkt = DummyPacketBuilder::BuildChatRequestPacket(dummy->m_usExpectedRoomNum);

				SendToDummy(dummy, chatPkt, DummyStatus::IN_ROOM_WAIT_CHAT);

				//_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[ProcessDummyLogic] - INDEX : [%d], THREAD : [%u], Send Chatting, UserId : [%llu], RoomNum : [%hu], ActionTime : [%llu], curTime : [%llu]", threadNum, GetCurrentThreadId(), dummy->m_ullUserId, dummy->m_usExpectedRoomNum, dummy->m_ullNextActionTime, curTimeStamp);
			}

			break;
		}
		case DummyStatus::CHECK_RTT:
		{
			PacketPtr echoPkt = DummyPacketBuilder::BuildEchoPacket();

			SendToDummy(dummy, echoPkt);

			break;
		}
		default:break;
		}
	}

}

void jh_content::DummyUpdateSystem::SendToDummy(DummyPtr& dummy, PacketPtr& packet, DummyStatus nextStauts)
{
	dummy->m_ullLastSendTime = jh_utility::GetTimeStamp();
	
	m_pOwner->SendPacket(dummy->m_ullSessionId, packet);

	dummy->m_ullNextActionTime = ULLONG_MAX;

	if (DummyStatus::NO_CHANGE != nextStauts)
		dummy->m_dummyStatus = nextStauts;
}

void jh_content::DummyUpdateSystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet, int threadNum)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return;

	if (false == IsValidThreadNum(threadNum))
		return;

	return (this->*m_packetFuncDic[packetType])(sessionId, packet, threadNum);
}

void jh_content::DummyUpdateSystem::HandleRoomListResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;
	

	DummyPtr dummy = m_logicData[threadNum].m_dummyUmap[sessionId];
	// LOBBY 
	if (DummyStatus::IN_LOBBY != dummy->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleRoomListResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		
		return;
	}

	USHORT roomCnt;

	*packet >> roomCnt;

	RoomInfo* roomInfo = static_cast<RoomInfo*>(g_memSystem->Alloc(sizeof(RoomInfo) * roomCnt));

	for (int i = 0; i < roomCnt; i++)
	{
		*packet >> roomInfo[i];
	}
	// roomCnt 0이면 Make
	if (roomCnt == 0)
	{
		PacketPtr makeRoomPkt = jh_content::DummyPacketBuilder::BuildMakeRoomRequestPacket();

		SendToDummy(dummy, makeRoomPkt);
	}
	else
	{
		int ran = GetRandValue(250, 0);
		int randRoomNum = GetRandValue(roomCnt, 0);

		// 확률 Make,
		if (ran < 2)
		{
			PacketPtr makeRoomPkt = jh_content::DummyPacketBuilder::BuildMakeRoomRequestPacket();

			SendToDummy(dummy, makeRoomPkt);

		}
		// 방 Enter
		else
		{
			PacketPtr enterRoomPkt = jh_content::DummyPacketBuilder::BuildEnterRoomRequestPacket(roomInfo[randRoomNum].m_usRoomNum, roomInfo[randRoomNum].m_wszRoomName);

			dummy->m_usExpectedRoomNum = roomInfo[randRoomNum].m_usRoomNum;
			wcscpy_s(dummy->m_wszExpectedRoomName, roomInfo[randRoomNum].m_wszRoomName);
			
			SendToDummy(dummy, enterRoomPkt, DummyStatus::WAIT_ENTER_ROOM);
		}
	}

	g_memSystem->Free(roomInfo);
}


	// Make, Enter 중 선택}

void jh_content::DummyUpdateSystem::HandleLogInResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;

	if (m_logicData[threadNum].m_dummyUmap.end() == m_logicData[threadNum].m_dummyUmap.find(sessionId))
		return;

	// LOGIN
	if (DummyStatus::SESSION_CONNECTED != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleLogInResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
	
		return;
	}

	static thread_local ULONGLONG cnt = 0;

	cnt++;
	if (1 == threadNum && 1 == cnt)
	{
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::CHECK_RTT;
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + 1000;

		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleLogInResponsePacket] - CHANGE Dummy Status : LOGIN -> CHECK_RTT, SessionID : [%llu]", sessionId);
	
	}
	else
	{
		ULONGLONG userId;

		*packet >> userId;

		m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullUserId = userId;
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_LOBBY;
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();
	}

}

void jh_content::DummyUpdateSystem::HandleMakeRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;
	
	DummyPtr dummy = m_logicData[threadNum].m_dummyUmap[sessionId];
	// LOBBY
	if (DummyStatus::IN_LOBBY != dummy->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleMakeRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		
		return;
	}

	jh_network::MakeRoomResponsePacket responsePkt;

	*packet >> responsePkt;

	if (false == responsePkt.isMade)
	{
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();


		return;
	}
	dummy->m_usExpectedRoomNum = responsePkt.roomInfo.m_usRoomNum;
	wcscpy_s(dummy->m_wszExpectedRoomName, responsePkt.roomInfo.m_wszRoomName);
	dummy->m_dummyStatus = DummyStatus::WAIT_ENTER_ROOM;
	
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = ULLONG_MAX;


}

void jh_content::DummyUpdateSystem::HandleEnterRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;

	// MAKE
	if (DummyStatus::WAIT_ENTER_ROOM != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleEnterRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		
		return;
	}

	jh_network::EnterRoomResponsePacket responsePkt;

	bool bAllow;
	*packet >> bAllow; // 나머지 정보는 받지 않음

	if(true == bAllow)
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_ROOM;
	else
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_LOBBY;

	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();

}

void jh_content::DummyUpdateSystem::HandleChatNotifyPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;

	const DummyStatus flag = (DummyStatus)((USHORT)DummyStatus::WAIT_LEAVE_ROOM | (USHORT)DummyStatus::IN_ROOM | (USHORT)DummyStatus::IN_ROOM_WAIT_CHAT);
	if (((USHORT)flag & (USHORT)m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus) != 0)
	{
		return;
	}
	_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatNotifyPacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);

	// ROOM
}

void jh_content::DummyUpdateSystem::HandleChatResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;

	if (DummyStatus::IN_ROOM_WAIT_CHAT != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleMakeRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);

		return;
	}
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_ROOM;
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();
}

void jh_content::DummyUpdateSystem::HandleLeaveRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;

	// ROOM
	if (DummyStatus::WAIT_LEAVE_ROOM != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleMakeRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);

		return;
	}

	m_logicData[threadNum].m_dummyUmap[sessionId]->m_usExpectedRoomNum = 0;
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_wszExpectedRoomName[0] = L'\0';

	m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_LOBBY;
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();

}
void jh_content::DummyUpdateSystem::HandleErrorPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	PRO_START_AUTO_FUNC;

	USHORT PacketErrorCode;

	*packet >> PacketErrorCode;

	DummyUpdateSystem::LogicData::ErrorAggregator& agg = m_logicData[threadNum].m_errorAggregator;

	switch (PacketErrorCode)
	{
	case jh_network::PacketErrorCode::REQUEST_DESTROYED_ROOM: InterlockedIncrement(&agg.m_llDestroyedRoom); break;
	case jh_network::PacketErrorCode::REQUEST_DIFF_ROOM_NAME: InterlockedIncrement(&agg.m_llDiffRoomName); break;
	case jh_network::PacketErrorCode::FULL_ROOM: InterlockedIncrement(&agg.m_llFullRoom); break;
	case jh_network::PacketErrorCode::ALREADY_RUNNING_ROOM: InterlockedIncrement(&agg.m_llAlreadyRunningRoom); break;
	}

	m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_LOBBY;
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();

}

void jh_content::DummyUpdateSystem::HandleEchoPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	// RTT 측정은 하나의 세션이 담당한다..
	PRO_START_AUTO_FUNC;

	ULONGLONG curTime = jh_utility::GetTimeStamp();

	DummyPtr& dummy = m_logicData[threadNum].m_dummyUmap[sessionId];
	if (DummyStatus::CHECK_RTT != dummy->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleEchoPacket] - DummyStatus is not \"CHECK_RTT\". DummyStatus : [%d]",sessionId, (int)(m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus));

		return;
	}
	
	InterlockedExchange64((LONGLONG*)& m_ullRtt, curTime - dummy->m_ullLastSendTime);

	dummy->m_ullNextActionTime = curTime + 1000;
}

void jh_content::DummyUpdateSystem::CheckSendTimeOut(int threadNum)
{
	PRO_START_AUTO_FUNC;

	std::vector<DummyPtr>& dummyVec = m_logicData[threadNum].m_dummyVector;

	ULONGLONG curTime = jh_utility::GetTimeStamp();
	
	int resendTimeoutCnt = 0;
	for (DummyPtr& dummy : dummyVec)
	{
		if ((curTime - dummy->m_ullLastSendTime) > RE_SEND_TIMEOUT)
		{
			resendTimeoutCnt++;

			_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"Thread Num  : [%u], Userid : [%llu], status : [%hu]", threadNum, dummy->m_ullUserId, (USHORT)dummy->m_dummyStatus);

		}
	}

	_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"Thread Num  : [%u], Total : [%d]" , threadNum, resendTimeoutCnt);
	InterlockedExchange(&m_logicData[threadNum].m_reSendTimeoutCnt, resendTimeoutCnt);
}


ULONGLONG jh_content::DummyUpdateSystem::GetRTT() const
{
	ULONGLONG rtt = m_ullRtt;
	_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[GetRTT] RTT : [%llu ms]", rtt);

	return rtt;
}


jh_content::DummyUpdateSystem::EtcData jh_content::DummyUpdateSystem::UpdateEtc()
{
	EtcData etc{};

	for (int i = 0; i < LOGIC_THREAD_COUNT; i++)
	{
		etc.m_lReSendTimeoutCount += InterlockedExchange(&m_logicData[i].m_reSendTimeoutCnt, 0);

		etc.m_llDestroyedRoomErrorCount += InterlockedExchange(&m_logicData[i].m_errorAggregator.m_llDestroyedRoom, 0);
		etc.m_llDiffRoomNameErrorCount += InterlockedExchange(&m_logicData[i].m_errorAggregator.m_llDiffRoomName, 0);
		etc.m_llFullRoomErrorCount += InterlockedExchange(&m_logicData[i].m_errorAggregator.m_llFullRoom, 0);
		etc.m_llAlreadyRunningRoomErrorCount += InterlockedExchange(&m_logicData[i].m_errorAggregator.m_llAlreadyRunningRoom, 0);
	}

	return etc;
}