#include "pch.h"
#include "EchoSystem.h"
#include "UserManager.h"

jh_content::EchoSystem::EchoSystem(jh_network::IocpServer* owner) : m_pOwner(owner), m_hLogicThread(nullptr), m_bRunningFlag(true), m_netJobQueue{}, m_sessionConnEventQueue{}
{
	if (nullptr == m_pOwner)
	{
		_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"EchoSystem() - Owner (Server) is NULL");
		jh_utility::CrashDump::Crash();
	}

	m_pUserManager = std::unique_ptr<jh_content::UserManager>();
}

jh_content::EchoSystem::~EchoSystem()
{
}

void jh_content::EchoSystem::Init()
{
	m_pUserManager->Init();

	m_packetFuncsDic.clear();

	m_packetFuncsDic[0] = &EchoSystem::ProcessEchoPacket;

	LPVOID param = this;
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, EchoSystem::LogicFunc, param, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"Echo System - Logic Handle is NULL");
		jh_utility::CrashDump::Crash();
	}

}

unsigned WINAPI jh_content::EchoSystem::LogicFunc(LPVOID lparam)
{
	EchoSystem* instance = reinterpret_cast<EchoSystem*>(lparam);

	instance->LobbyLogic();

	return 0;
}

void jh_content::EchoSystem::LobbyLogic()
{
	while (true == m_bRunningFlag)
	{
		// Packet 관련 작업 처리
		ProcessNetJob();

		// System 관련 작업 처리.
		ProcessSystemJob();

		// 채팅이기 때문에 들어오는대로 바로 작업을 처리해도 괜찮아보임.
	}
}

void jh_content::EchoSystem::Stop()
{
	m_bRunningFlag = false;

	DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

	if (ret != WAIT_OBJECT_0)
	{
		DWORD error = GetLastError();
		_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"Echo System Stop - WaitForSingleObject : %u, GetLastError : %u", ret, error);
		jh_utility::CrashDump::Crash();
		return;
	}

	_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"Echo Thread Shutdown");
}

void jh_content::EchoSystem::ProcessNetJob()
{
	static std::vector<JobPtr> jobList;

	m_netJobQueue.PopAll(jobList);

	for (JobPtr& job : jobList)
	{
		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
	}
	jobList.clear();

	// netJobQueue에서 Job 확인 후 
	// 작업따라 패킷 처리

}

void jh_content::EchoSystem::ProcessSystemJob()
{
	static std::vector<SessionConnectionEventPtr> systemJobList;

	m_sessionConnEventQueue.PopAll(systemJobList);

	for (SessionConnectionEventPtr& job : systemJobList)
	{
		switch (job->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{
			//m_pUserManager->CreateUser(job->m_llSessionId);
			// 유저 접속 처리.
		}
		break;
		{
			//m_pUserManager->RemoveUser(job->m_llSessionId);
			// 유저 접속 종료 처리.
		}
		break;
		}
	}

	systemJobList.clear();
}

//ErrorCode jh_content::EchoSystem::ProcessEchoPacket(LONGLONG sessionId, jh_network::SerializationBufferPtr& serializationBufferPtr)
ErrorCode jh_content::EchoSystem::ProcessEchoPacket(LONGLONG sessionId, PacketPtr& packet)
{
	short len;
	ULONGLONG data;

	*packet >> data;
	//_LOG(m_pOwner->GetServerName(), LOG_LEVEL_DETAIL, L"ProcessEchoPacket - data : %u", data);

	PacketPtr pPacket = jh_content::PacketBuilder::BuildEchoPacket(8, data);

	m_pOwner->SendPacket(sessionId, pPacket);

	return ErrorCode::NONE;
}
