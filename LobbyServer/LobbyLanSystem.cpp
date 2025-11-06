#include "pch.h"
#include "LobbyLanSystem.h"
#include "LobbySystem.h"
#include "Memory.h"


jh_content::LobbyLanSystem::~LobbyLanSystem()
{
	if (nullptr != m_hLogicThread)
	{
		Stop();
	}
}

ErrorCode jh_content::LobbyLanSystem::ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet)
{
	if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
		return ErrorCode::CANNOT_FIND_PACKET_FUNC;

	return (this->*m_packetFuncDic[packetType])(sessionId, packet);
}

void jh_content::LobbyLanSystem::ProcessNetJob()
{
	static thread_local alignas(64) std::queue<JobPtr> jobQ;

	m_netJobQueue.Swap(jobQ);

	while(jobQ.size() > 0)
	{
		JobPtr& job = jobQ.front();

		ProcessPacket(job->m_llSessionId, job->m_wJobType, job->m_pPacket);
		
		jobQ.pop();
	}
}

unsigned __stdcall jh_content::LobbyLanSystem::LogicThreadFunc(LPVOID lparam)
{
	jh_content::LobbyLanSystem* lobbyLanInstance = static_cast<LobbyLanSystem*>(lparam);

	if (nullptr == lobbyLanInstance)
		return 0;

	lobbyLanInstance->LobbyLanLogic();

	return 0;
}


void jh_content::LobbyLanSystem::LobbyLanLogic()
{
	while (true == m_bRunningFlag.load())
	{
		WaitForSingleObject(m_hJobEvent, 1000);
		// Packet 관련 작업 처리
		ProcessNetJob();
	}
}

void jh_content::LobbyLanSystem::Stop()
{
	m_bRunningFlag.store(false);

	SetEvent(m_hJobEvent);

	if (nullptr != m_hLogicThread)
	{
		DWORD ret = WaitForSingleObject(m_hLogicThread, INFINITE);

		if (ret != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();

			_LOG(LOBBY_LAN_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Stop] Logic thread wait failed. GetLastError: [%u]", gle);

			jh_utility::CrashDump::Crash();
		}

		CloseHandle(m_hLogicThread);
		CloseHandle(m_hJobEvent);

		m_hJobEvent = nullptr;
		m_hLogicThread = nullptr;
		m_netJobQueue.Clear();
	}
}

void jh_content::LobbyLanSystem::Init()
{
	m_packetFuncDic.clear();

	m_packetFuncDic[jh_network::GAME_SERVER_LAN_INFO_PACKET] = &LobbyLanSystem::HandleLanInfoNotifyPacket; // ip Port
	m_packetFuncDic[jh_network::GAME_SERVER_SETTING_REQUEST_PACKET] = &LobbyLanSystem::HandleGameSettingRequestPacket; // completePacket

	m_hJobEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == m_hJobEvent)
	{
		DWORD gle = GetLastError();
		_LOG(LOBBY_LAN_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Init] JobEvent handle is nullptr. Error: [%u]", gle);

		jh_utility::CrashDump::Crash();
	}

	LPVOID param = this;
	m_hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LobbyLanSystem::LogicThreadFunc, param, 0, nullptr));

	if (nullptr == m_hLogicThread)
	{
		_LOG(LOBBY_LAN_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[Init] Logic thread handle is nullptr.");
		jh_utility::CrashDump::Crash();
	}
}

ErrorCode jh_content::LobbyLanSystem::HandleLanInfoNotifyPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	LanRequestPtr lanRequest = MakeShared<LanRequest>(g_memSystem, sessionId, jh_network::GAME_SERVER_LAN_INFO_PACKET, packet, m_pOwner);

	m_pLobbySystem->EnqueueLanRequest(lanRequest);

	return ErrorCode::NONE;
}

ErrorCode jh_content::LobbyLanSystem::HandleGameSettingRequestPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	LanRequestPtr lanRequest = MakeShared<LanRequest>(g_memSystem, sessionId, jh_network::GAME_SERVER_SETTING_REQUEST_PACKET, packet, m_pOwner);

	m_pLobbySystem->EnqueueLanRequest(lanRequest);

	return ErrorCode::NONE;
}
