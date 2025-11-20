#include "pch.h"
#include "EchoSystem.h"
#include "UserManager.h"

jh_content::EchoSystem::EchoSystem(jh_network::IocpServer* owner) : m_pOwner(owner), m_hLogicThread(nullptr), m_bRunningFlag(true), m_netJobQueue{}, m_sessionConnEventQueue{}
{
	if (nullptr == m_pOwner)
	{
		_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[EchoSystem] m_pOnwer is nullptr.");
		jh_utility::CrashDump::Crash();
	}
}

jh_content::EchoSystem::~EchoSystem()
{
}

void jh_content::EchoSystem::Init()
{
	m_packetFuncDic.clear();

	m_packetFuncDic[0] = &EchoSystem::ProcessEchoPacket;

	LPVOID param = this;
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, EchoSystem::LogicFunc, param, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[EchoSystem] LogicThread handle is nullptr.");
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
		// Packet 包访 累诀 贸府
		ProcessNetJob();

		// System 包访 累诀 贸府.
		ProcessSystemJob();

	}
}

void jh_content::EchoSystem::Stop()
{
	m_bRunningFlag = false;

	DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

	if (ret != WAIT_OBJECT_0)
	{
		DWORD gle = GetLastError();
		_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Stop] WaitForSingleObject failed. Ret: %u, GetLastError: %u", ret, gle);
		jh_utility::CrashDump::Crash();
		return;
	}

	_LOG(ECHO_SYSTEM_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"[STOP] EchoThread Shutdown.");
}

void jh_content::EchoSystem::ProcessNetJob()
{
	static thread_local alignas(64) std::queue<JobRef> echoJobQ;

	m_netJobQueue.Swap(echoJobQ);

	while(echoJobQ.size() > 0)
	{
		JobRef& job = echoJobQ.front();

		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
		
		echoJobQ.pop();
	}
}

void jh_content::EchoSystem::ProcessSystemJob()
{
	static thread_local alignas(64) std::queue<SessionConnectionEventRef> systemJobQ;
	std::queue<SessionConnectionEventRef> emptyQ;

	m_sessionConnEventQueue.Swap(systemJobQ);

	while(systemJobQ.size() > 0)
	{
		SessionConnectionEventRef& job = systemJobQ.front();

		switch (job->m_msgType)
		{
		case jh_utility::SessionConnectionEventType::CONNECT:
		{
			//m_pUserManager->CreateUser(job->m_llSessionId);
			// 蜡历 立加 贸府.
		}
		break;
		{
			//m_pUserManager->RemoveUser(job->m_llSessionId);
			// 蜡历 立加 辆丰 贸府.
		}
		break;
		}
		
		systemJobQ.pop();
	}
}

void jh_content::EchoSystem::ProcessEchoPacket(LONGLONG sessionId, PacketBufferRef& packet)
{
	short len;
	ULONGLONG data;

	*packet >> data;

	PacketBufferRef pPacket = jh_content::PacketBuilder::BuildEchoPacket(8, data);

	m_pOwner->SendPacket(sessionId, pPacket);
}
