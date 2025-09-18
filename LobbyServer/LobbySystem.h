#pragma once

namespace jh_content
{	
	class LobbySystem
	{
		//using PacketFunc = ErrorCode(LobbySystem::*)(SessionPtr&, jh_network::SerializationBufferPtr&);
		using PacketFunc = ErrorCode(LobbySystem::*)(ULONGLONG, PacketPtr&);

	public:
		LobbySystem(jh_network::IocpServer* owner, USHORT maxRoomCnt, USHORT maxRoomUserCnt);
		~LobbySystem();

		// 세션에서의 Connect / Disconnect 
		// Game에서의 Connect / Disconnect
		// 작업도 분리
		void EnqueueJob(JobPtr& job)
		{
			m_netJobQueue.Push(job);
		}

		void EnqueueSessionConnEvent(SessionConnectionEventPtr& sessionConnEventPtr)
		{
			m_sessionConnEventQueue.Push(sessionConnEventPtr);
		}

		void EnqueueLanRequest(LanRequestPtr& lanRequestPtr)
		{
			m_lanRequestQueue.Push(lanRequestPtr);
		}

		static unsigned WINAPI StaticLogicProxy(LPVOID lparam);
		void LobbyLogic();

		void Init();
		// 로직 스레드의 실행을 종료
		void Stop();

	private:
		ErrorCode ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet);

		void ProcessNetJob();
		void ProcessSessionConnectionEvent();
		void ProcessLanRequest();

		// 함수 정의
		ErrorCode HandleRoomListRequestPacket(ULONGLONG sessionId,PacketPtr& packet);

		ErrorCode HandleLogInRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		ErrorCode HandleChatToRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		ErrorCode HandleMakeRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		ErrorCode HandleEnterRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		ErrorCode HandleLeaveRoomRequestPacket(ULONGLONG sessionId, PacketPtr& packet);
		ErrorCode HandleGameReadyRequestPacket(ULONGLONG sessionId, PacketPtr& packet);

		ErrorCode HandleHeartbeatPacket(ULONGLONG sessionId, PacketPtr& packet);


		// Lan Request
		void HandleLanInfoNotify(ULONGLONG lanSessionId, PacketPtr& lanPacket, jh_network::IocpServer* lanServer);
		void HandleGameSettingRequest(ULONGLONG lanSessionId, PacketPtr& lanPacket, jh_network::IocpServer* lanServer);
		
		std::unordered_map<USHORT, PacketFunc> m_packetFuncsDic;

		std::unique_ptr<UserManager> m_pUserManager;
		std::unique_ptr<jh_content::RoomManager> m_pRoomManager;

		jh_utility::LockQueue<JobPtr> m_netJobQueue;
		jh_utility::LockQueue<SessionConnectionEventPtr> m_sessionConnEventQueue;
		jh_utility::LockQueue<LanRequestPtr> m_lanRequestQueue;

		std::vector<USHORT> m_pendingGameRoomList;

		HANDLE m_hLogicThread;
		jh_network::IocpServer* m_pOwner;
		std::atomic<bool> m_bRunningFlag;


	};
}