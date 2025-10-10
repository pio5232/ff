#include "pch.h"
#include "DummyUpdateSystem.h"
#include "DummyPacketBuilder.h"
#include "Memory.h"
unsigned jh_content::DummyUpdateSystem::StaticLogic(LPVOID lparam)
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
	const HANDLE jobEvent = m_logicData[threadNum].m_hJobEvent;
	std::vector<DummyPtr>& dummyVec = m_logicData[threadNum].m_dummyVector;
	
	while (true == m_bRunnigFlag.load())
	{
		WaitForSingleObject(jobEvent, 1000);

		ProcessNetJob(threadNum);

		ProcessSessionConnectionEvent(threadNum);

		ULONGLONG curTimeStamp = jh_utility::GetTimeStamp();

		for (DummyPtr& dummy : dummyVec)
		{
			if (curTimeStamp - dummy->m_ullLastUpdatedHeartbeatTime > 5000)
			{
				dummy->m_ullLastUpdatedHeartbeatTime = curTimeStamp;

				PacketPtr hbPkt = jh_content::DummyPacketBuilder::BuildHeartbeatPacket(curTimeStamp);
				m_pOwner->SendPacket(dummy->m_ullSessionId, hbPkt);
			}

			if (false == clientSendFlag.load())
				continue;

			if (dummy->m_ullNextActionTime < curTimeStamp)
				continue;

			switch (dummy->m_dummyStatus)
			{
			case DummyStatus::IN_LOBBY:
			{
				PacketPtr roomListRqPkt = DummyPacketBuilder::BuildRoomListRequestPacket(); 
				m_pOwner->SendPacket(dummy->m_ullSessionId, roomListRqPkt);

				dummy->m_ullNextActionTime = MAXULONGLONG;

				break;
			}
			case DummyStatus::IN_ROOM:
			{
				break;
			}
			}
		}

	}

	
}


jh_content::DummyUpdateSystem::DummyUpdateSystem(jh_network::IocpClient* owner) : m_pOwner{ owner }, m_bRunnigFlag{ true }
{

}

jh_content::DummyUpdateSystem::~DummyUpdateSystem()
{
	if (false != m_bRunnigFlag)
	{
		Stop();
	}
}

void jh_content::DummyUpdateSystem::Init()
{
	m_packetFuncDic.clear();

	m_packetFuncDic[jh_network::ROOM_LIST_RESPONSE_PACKET] = &DummyUpdateSystem::HandleRoomListResponsePacket;
	m_packetFuncDic[jh_network::CHAT_NOTIFY_PACKET] = &DummyUpdateSystem::HandleChatNotifyPacket;
	m_packetFuncDic[jh_network::LOG_IN_RESPONSE_PACKET] = &DummyUpdateSystem::HandleLogInResponsePacket;
	m_packetFuncDic[jh_network::MAKE_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleMakeRoomResponsePacket;
	m_packetFuncDic[jh_network::ENTER_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleEnterRoomResponsePacket;
	m_packetFuncDic[jh_network::LEAVE_ROOM_RESPONSE_PACKET] = &DummyUpdateSystem::HandleLeaveRoomResponsePacket;

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

		logicData.m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, DummyUpdateSystem::StaticLogic, &transData[i], 0, nullptr));

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
	m_bRunnigFlag.store(false);
	
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
	static thread_local alignas(64) std::vector<JobPtr> jobList;

	std::vector<JobPtr>	emptyVec;

	m_logicData[threadNum].m_netJobQueue.PopAll(jobList);

	for (JobPtr& job : jobList)
	{
		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket, threadNum);
	}
	
	jobList.swap(emptyVec);
}

void jh_content::DummyUpdateSystem::ProcessSessionConnectionEvent(int threadNum)
{
	LogicData& threadLogicData = m_logicData[threadNum];

	static thread_local alignas(64) std::vector<SessionConnectionEventPtr> sessionConnEventList;
	std::vector<SessionConnectionEventPtr> emptyList;

	threadLogicData.m_sessionConnEventQueue.PopAll(sessionConnEventList);
	
	for (SessionConnectionEventPtr& sessionConnEvent : sessionConnEventList)
	{
		ULONGLONG sessionId = sessionConnEvent->m_ullSessionId;

		switch (sessionConnEvent->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{
			//DummyData dummy{ .m_dummyStatus{DummyStatus::SESSION_CONNECTED}, .m_ullSessionId{sessionId}, .m_ullLastUpdatedHeartbeatTime{jh_utility::GetTimeStamp()} };
			
			DummyPtr dummy = MakeShared<DummyData>(g_memAllocator);
			
			dummy->m_dummyStatus = DummyStatus::SESSION_CONNECTED;
			dummy->m_ullSessionId = sessionId;
			dummy->m_ullLastUpdatedHeartbeatTime = jh_utility::GetTimeStamp();

			threadLogicData.m_dummyUmap[sessionId] = dummy;
			threadLogicData.m_dummyVector.push_back(dummy);
			
			int idx = static_cast<int>(threadLogicData.m_dummyVector.size() - 1);
			
			threadLogicData.m_dummyVectorIndexUMap[dummy] = idx;

			// 등록
			PacketPtr hbPkt = jh_content::DummyPacketBuilder::BuildHeartbeatPacket(dummy->m_ullLastUpdatedHeartbeatTime);
			m_pOwner->SendPacket(sessionId, hbPkt);

			PacketPtr logInReqPkt = jh_content::DummyPacketBuilder::BuildLoginRequestPacket();
			m_pOwner->SendPacket(sessionId, logInReqPkt);
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
	}

	sessionConnEventList.swap(emptyList);
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
	// LOBBY 
	if (DummyStatus::IN_LOBBY != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleRoomListResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		return;
	}

	USHORT roomCnt;

	*packet >> roomCnt;

	RoomInfo* roomInfo = static_cast<RoomInfo*>(g_memAllocator->Alloc(sizeof(RoomInfo) * roomCnt));
	
	for (int i = 0; i < roomCnt; i++)
	{
		*packet >> roomInfo[i];
	}
	// roomCnt 0이면 Make
	if (roomCnt == 0)
	{
		PacketPtr makeRoomPkt = jh_content::DummyPacketBuilder::BuildMakeRoomRequestPacket();
		m_pOwner->SendPacket(sessionId, makeRoomPkt);
	}
	else
	{
		int ran = GetRandValue(250, 0);
		int randRoomNum = GetRandValue(roomCnt, 0);

		// 확률 Make,
		if (ran < 1)
		{
			PacketPtr makeRoomPkt = jh_content::DummyPacketBuilder::BuildMakeRoomRequestPacket();
			m_pOwner->SendPacket(sessionId, makeRoomPkt);
		}
		// 방 Enter
		else
		{
			PacketPtr enterRoomPkt = jh_content::DummyPacketBuilder::BuildEnterRoomRequestPacket(roomInfo[randRoomNum].m_usRoomNum, roomInfo[randRoomNum].m_wszRoomName);
			m_pOwner->SendPacket(sessionId, enterRoomPkt);
		}
	}
	
	// Make, Enter 중 선택
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = MAXULONGLONG;
}

void jh_content::DummyUpdateSystem::HandleLogInResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	if (m_logicData[threadNum].m_dummyUmap.end() == m_logicData[threadNum].m_dummyUmap.find(sessionId))
		return;

	// LOGIN
	if (DummyStatus::SESSION_CONNECTED != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleLogInResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		return;
	}


	m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_LOBBY;
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();
}

void jh_content::DummyUpdateSystem::HandleMakeRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	//// LOBBY
	if (DummyStatus::IN_LOBBY != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleMakeRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		return;
	}

	jh_network::MakeRoomResponsePacket responsePkt;

	*packet >> responsePkt;

	if (false == responsePkt.isMade)
		return;


}

void jh_content::DummyUpdateSystem::HandleEnterRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	// LOBBY || MAKE
	if (DummyStatus::IN_LOBBY != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleEnterRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		return;
	}

	jh_network::EnterRoomResponsePacket responsePkt;

	bool bAllow;
	*packet >> bAllow; // 나머지 정보는 받지 않음

	if(true == bAllow)
		m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_ROOM;
	
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();
}

void jh_content::DummyUpdateSystem::HandleChatNotifyPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	if (DummyStatus::IN_ROOM != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleChatNotifyPacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		return;
	}

	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();

	// ROOM
}

void jh_content::DummyUpdateSystem::HandleLeaveRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum)
{
	// ROOM
	if (DummyStatus::IN_ROOM != m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus)
	{
		_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[HandleMakeRoomResponsePacket] - m_dummyStatus가 올바르지 않습니다 Session : [%llu], status : [%d]", sessionId, m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus);
		return;
	}

	m_logicData[threadNum].m_dummyUmap[sessionId]->m_dummyStatus = DummyStatus::IN_LOBBY;
	m_logicData[threadNum].m_dummyUmap[sessionId]->m_ullNextActionTime = jh_utility::GetTimeStamp() + GetRandTimeForDummy();

}
