#pragma once

namespace jh_content
{
	class LobbySystem;

	class LobbyLanSystem
	{
	public:
		using PacketFunc = ErrorCode(LobbyLanSystem::*)(ULONGLONG, PacketPtr&);
	
	public:	
		LobbyLanSystem(jh_network::IocpServer* owner) : m_pOwner(owner), m_hLogicThread(nullptr), m_netJobQueue(), m_pLobbySystem(nullptr), m_bRunningFlag(true), m_hJobEvent(nullptr) {}
		
		~LobbyLanSystem();
		
		void Init();

		void SetLobbySystem(class jh_content::LobbySystem* lobbySystem) { m_pLobbySystem = lobbySystem; }
	
		void EnqueueJob(JobPtr& job)
		{
			m_netJobQueue.Push(job);

			SetEvent(m_hJobEvent);
		}

		static unsigned WINAPI StaticLogic(LPVOID lparam);
		void LobbyLanLogic();
		void Stop();
		ErrorCode HandleLanInfoNotifyPacket(ULONGLONG sessionId, PacketPtr& packet);
		ErrorCode HandleGameSettingRequestPacket(ULONGLONG sessionId, PacketPtr& packet);

	private:
		ErrorCode ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet);
		void ProcessNetJob();

		std::unordered_map<USHORT, PacketFunc> m_packetFuncDic;

		jh_network::IocpServer* m_pOwner;
		class jh_content::LobbySystem* m_pLobbySystem;

		jh_utility::LockQueue<JobPtr> m_netJobQueue;

		std::atomic<bool> m_bRunningFlag;
		HANDLE m_hLogicThread;
		HANDLE m_hJobEvent;


	};
}

