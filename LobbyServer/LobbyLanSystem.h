#pragma once

namespace jh_content
{
	class LobbySystem;

	class LobbyLanSystem
	{
	public:
		using PacketFunc = void(LobbyLanSystem::*)(ULONGLONG, PacketBufferRef&);

	public:
		LobbyLanSystem(jh_network::IocpServer* owner) : m_pOwner{ owner }, m_hLogicThread{ nullptr }, m_netJobQueue{}, m_pLobbySystem{ nullptr }, m_bRunningFlag{ 1 }, m_hJobEvent{ nullptr } {}
		
		~LobbyLanSystem();
		
		void Init();

		void SetLobbySystem(class jh_content::LobbySystem* lobbySystem) { m_pLobbySystem = lobbySystem; }
	
		void EnqueueJob(JobRef& job)
		{
			m_netJobQueue.Push(job);

			SetEvent(m_hJobEvent);
		}

		static unsigned WINAPI LogicThreadFunc(LPVOID lparam);
		void LobbyLanLogic();
		void Stop();
		void HandleLanInfoNotifyPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleGameSettingRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);

	private:
		void ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketBufferRef& packet);
		void ProcessNetJob();

		char									m_bRunningFlag;
		std::unordered_map<USHORT, PacketFunc>	m_packetFuncDic;

		jh_network::IocpServer					* m_pOwner;
		class jh_content::LobbySystem			* m_pLobbySystem;

		jh_utility::LockQueue<JobRef>			m_netJobQueue;

		HANDLE									m_hLogicThread;
		HANDLE									m_hJobEvent;


	};
}

