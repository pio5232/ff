#pragma once

namespace jh_content
{	
	class LobbySystem
	{
		using PacketFunc = void(LobbySystem::*)(ULONGLONG, PacketBufferRef&);

	public:
		LobbySystem(jh_network::IocpServer* owner, USHORT maxRoomCnt, USHORT maxRoomUserCnt);
		~LobbySystem();

		// 세션에서의 Connect / Disconnect 
		// Game에서의 Connect / Disconnect
		// 작업도 분리
		void EnqueueJob(JobRef& job)
		{
			m_netJobQueue.Push(job);

			SetEvent(m_hJobEvent);
		}

		void EnqueueSessionConnEvent(SessionConnectionEventRef& sessionConnEventPtr)
		{
			m_sessionConnEventQueue.Push(sessionConnEventPtr);
			
			SetEvent(m_hJobEvent);
		}

		void EnqueueLanRequest(LanRequestPtr& lanRequestPtr)
		{
			m_lanRequestQueue.Push(lanRequestPtr);
			
			SetEvent(m_hJobEvent);
		}

		static unsigned WINAPI LogicThreadFunc(LPVOID lparam);
		void LogicThreadMain();

		void Init();
		// 로직 스레드의 실행을 종료
		void Stop();

		void GetInvalidMsgCnt();
		LONG GetJobCount() { m_netJobQueue.GetUseSize(); }
	private:
		LONGLONG m_invalidLeave = 0;
		LONGLONG m_invalidEnter = 0;
		LONGLONG m_invalidChat = 0;
		LONGLONG m_invalidMake = 0;
		LONGLONG m_invalidReady = 0;

		void ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketBufferRef& packet);

		void ProcessNetJob();
		void ProcessSessionConnectionEvent();
		void ProcessLanRequest();

		// 패킷 처리 함수
		void HandleRoomListRequestPacket(ULONGLONG sessionId,PacketBufferRef& packet);
		void HandleLogInRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleChatToRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleMakeRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleEnterRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleLeaveRoomRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleGameReadyRequestPacket(ULONGLONG sessionId, PacketBufferRef& packet);
		void HandleEchoPacket(ULONGLONG sessionId, PacketBufferRef& packet);

		void HandleHeartbeatPacket(ULONGLONG sessionId, PacketBufferRef& packet);

		// Lan Request
		void HandleLanInfoNotify(ULONGLONG lanSessionId, PacketBufferRef& lanPacket, jh_network::IocpServer* lanServer);
		void HandleGameSettingRequest(ULONGLONG lanSessionId, PacketBufferRef& lanPacket, jh_network::IocpServer* lanServer);
		
		char												m_bRunningFlag;
		std::unordered_map<USHORT, PacketFunc>				m_packetFuncDic;

		std::unique_ptr<UserManager>						m_pUserManager;
		std::unique_ptr<jh_content::RoomManager>			m_pRoomManager;

		jh_utility::LockQueue<JobRef>						m_netJobQueue;
		jh_utility::LockQueue<SessionConnectionEventRef>	m_sessionConnEventQueue;
		jh_utility::LockQueue<LanRequestPtr>				m_lanRequestQueue;
		
		std::vector<USHORT>									m_pendingGameRoomList;

		HANDLE												m_hLogicThread;
		HANDLE												m_hJobEvent;
		jh_network::IocpServer								* m_pOwner;


	};
}