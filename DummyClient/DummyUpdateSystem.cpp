#include "pch.h"
#include "DummyUpdateSystem.h"

unsigned jh_content::DummyUpdateSystem::Worker(LPVOID lparam)
{
	WorkerTransData* data = static_cast<WorkerTransData*>(lparam);

	DummyUpdateSystem* dummyInstance = static_cast<DummyUpdateSystem*>(data->m_pThis);
	
	if (nullptr == dummyInstance)
		return 0;

	dummyInstance->WorkerThread(data->m_iThreadNum);

	return 0;
}

void jh_content::DummyUpdateSystem::WorkerThread(int threadNum)
{
	const HANDLE jobEvent = m_logicData[threadNum].m_hJobEvent;
	
	while (true == m_bRunnigFlag.load())
	{
		WaitForSingleObject(jobEvent, INFINITE);

		ProcessNetJob();
	}
}


jh_content::DummyUpdateSystem::DummyUpdateSystem(jh_network::IocpClient* owner) : m_pOwner{ owner }, m_bRunnigFlag{ true }
{

}

jh_content::DummyUpdateSystem::~DummyUpdateSystem()
{
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

		logicData.m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, DummyUpdateSystem::Worker, &transData[i], 0, nullptr));

		if (nullptr == logicData.m_hLogicThread)
		{
			_LOG(LOBBY_DUMMY_SAVEFILE_NAME, LOG_LEVEL_WARNING, L"[DummyUpdateSystem] - 로직 핸들이 존재하지 않습니다. 인덱스 : [%d]",i);

			jh_utility::CrashDump::Crash();
		}
	}
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
}

void jh_content::DummyUpdateSystem::EnqueueJob(JobPtr& job, int idx)
{
	if (idx < 0 || idx > LOGIC_THREAD_COUNT)
		return;

	LogicData& logicData = m_logicData[idx];

	logicData.m_netJobQueue.Push(job);

	SetEvent(logicData.m_hJobEvent);
}

void jh_content::DummyUpdateSystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return;

	return (this->*m_packetFuncDic[packetType])(sessionId, packet);
}

void jh_content::DummyUpdateSystem::HandleRoomListResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::DummyUpdateSystem::HandleLogInResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::DummyUpdateSystem::HandleMakeRoomResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::DummyUpdateSystem::HandleEnterRoomResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::DummyUpdateSystem::HandleChatNotifyPacket(ULONGLONG session, PacketPtr& packet)
{
}

void jh_content::DummyUpdateSystem::HandleLeaveRoomResponsePacket(ULONGLONG session, PacketPtr& packet)
{
}
