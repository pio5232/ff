#pragma once

namespace jh_content
{
	class GameWorld;
	class UserManager;

	struct GameServerInfo
	{
		ULONGLONG m_ullEnterToken;
		USHORT m_usRoomNumber;
		//USHORT m_usPort;

		USHORT m_usRequiredUserCnt;
		USHORT m_usMaxUserCnt;
	};

	class GameSystem
	{
		using PacketFunc = void(GameSystem::*)(ULONGLONG, PacketPtr&);

	public:
		GameSystem(jh_network::IocpServer* owner);
		~GameSystem();


		USHORT GetRoomNumber() const { return m_gameInfo.m_usRoomNumber; }
		ULONGLONG GetToken() const { return m_gameInfo.m_ullEnterToken; }
		//USHORT GetRealPort() const { return m_gameInfo.m_usPort; }
		USHORT GetRequiredUsers() const { return m_gameInfo.m_usRequiredUserCnt; }
		USHORT MaxUsers() const { return m_gameInfo.m_usMaxUserCnt; }


		// Lan에서 정보를 받으면 갱신한다.
		void SetGameInfo(USHORT roomNumber, USHORT requiredUsers, USHORT maxUsers);

		void EnqueueJob(JobPtr& job)
		{
			m_netJobQueue.Push(job);
		}
		void EnqueueSessionConnEvent(SessionConnectionEventPtr& sessionConnectionEventPtr)
		{
			m_sessionConnEventQueue.Push(sessionConnectionEventPtr);
		}
		void EnqueueLanRequest(GameLanRequestPtr& lanRequestPtr)
		{
			m_gameLanRequestQueue.Push(lanRequestPtr);
		}
		void Init();
		void Stop();
	private:
		static unsigned StaticLogic(LPVOID lparam);
		void GameLogic();

		void ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet);

		void ProcessNetJob();
		void ProcessSessionConnectionEvent();
		void ProcessLanRequest();

		// GAME
		void HandleEnterGameRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		void HandleLoadCompletedPacket(ULONGLONG sessionId, PacketPtr& packet);

		void HandleMoveStartRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		void HandleMoveStopRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		void HandleChatRequestPacket(ULONGLONG sessionId, PacketPtr& packet);

		void HandleAttackRequestPacket(ULONGLONG sessionId, PacketPtr& packet);

		// LAN
		void HandleGameServerSettingResponsePacket(ULONGLONG lanSessionId, PacketPtr& packet, jh_network::IocpClient* lanClient);
	private:
		GameServerInfo m_gameInfo;

		std::atomic<bool> m_bIsRunning;
		USHORT m_dwLoadCompletedCnt;

		jh_utility::LockQueue<JobPtr> m_netJobQueue;
		jh_utility::LockQueue<SessionConnectionEventPtr> m_sessionConnEventQueue;
		jh_utility::LockQueue<GameLanRequestPtr> m_gameLanRequestQueue;
		
		std::unique_ptr<class jh_content::GameWorld> m_pGameWorld;
		std::unique_ptr<class jh_content::UserManager> m_pUserManager;

		std::unordered_map<USHORT, PacketFunc> m_packetFuncDic;
		
		jh_network::IocpServer* m_pOwner;
		HANDLE m_hLogicThread;
	};
}

