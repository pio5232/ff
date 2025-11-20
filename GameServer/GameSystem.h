#pragma once

namespace jh_content
{
	class GameWorld;
	class UserManager;

	struct GameServerInfo
	{
		ULONGLONG m_ullEnterToken;
		USHORT m_usRoomNumber;

		USHORT m_usRequiredUserCnt;
		USHORT m_usMaxUserCnt;
	};

	class GameSystem
	{
		using PacketFunc = void(GameSystem::*)(ULONGLONG, PacketBufferRef&);

	public:
		GameSystem(jh_network::IocpServer* owner);
		~GameSystem();

		USHORT GetRoomNumber() const { return m_gameInfo.m_usRoomNumber; }
		ULONGLONG GetToken() const { return m_gameInfo.m_ullEnterToken; }
		USHORT GetRequiredUsers() const { return m_gameInfo.m_usRequiredUserCnt; }
		USHORT MaxUsers() const { return m_gameInfo.m_usMaxUserCnt; }

		// Lan에서 정보를 받으면 갱신한다.
		void SetGameInfo(USHORT roomNumber, USHORT requiredUsers, USHORT maxUsers);

		void EnqueueJob(JobRef& job)
		{
			m_netJobQueue.Push(job);
		}
		void EnqueueSessionConnEvent(SessionConnectionEventRef& sessionConnectionEventPtr)
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
		static unsigned LogicThreadFunc(LPVOID lparam);
		void GameLogic();

		void ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketBufferRef& packet);

		void ProcessNetJob();
		void ProcessSessionConnectionEvent();
		void ProcessLanRequest();

		// GAME
		void HandleEnterGameRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleLoadCompletedPacket(ULONGLONG sessionId, PacketBufferRef& packet);

		void HandleMoveStartRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleMoveStopRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleChatRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);

		void HandleAttackRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);

		// LAN
		void HandleGameServerSettingResponsePacket(ULONGLONG lanSessionId, PacketBufferRef& packet, jh_network::IocpClient* lanClient);
	private:
		GameServerInfo										m_gameInfo;

		volatile char										m_bIsRunning;
		USHORT												m_usLoadCompletedCnt;

		jh_utility::LockQueue<JobRef> m_netJobQueue;
		jh_utility::LockQueue<SessionConnectionEventRef>	m_sessionConnEventQueue;
		jh_utility::LockQueue<GameLanRequestPtr>			m_gameLanRequestQueue;
		
		std::unique_ptr<class jh_content::GameWorld>		m_pGameWorld;
		std::unique_ptr<class jh_content::UserManager>		m_pUserManager;

		std::unordered_map<USHORT, PacketFunc>				m_packetFuncDic;
		
		jh_network::IocpServer								* m_pOwner;
		HANDLE												m_hLogicThread;
	};
}

