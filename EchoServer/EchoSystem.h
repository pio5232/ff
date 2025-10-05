#pragma once

namespace jh_content
{
	class EchoSystem
	{
		//using PacketFunc = ErrorCode(LobbySystem::*)(ULONGLONG, jh_network::SerializationBufferPtr&);
		// 어차피 수행하는 쪽에서 SessionPtr을 담아두고 보관한 상태에서 Process 호출 -> 사용하는 동안엔 제거되지 않는 것이 확실하다.
		//using PacketFunc = ErrorCode(EchoSystem::*)(LONGLONG, jh_utility::SerializationBuffer*);
		using PacketFunc = ErrorCode(EchoSystem::*)(LONGLONG, PacketPtr&);

	public:
		EchoSystem(jh_network::IocpServer* owner);
		~EchoSystem();

		void Init();

		inline void EnqueueJob(JobPtr& job)
		{
			m_netJobQueue.Push(job);
		}

		//inline JobPtr DequeueJob()
		//{
		//	return m_netJobQueue.Pop();
		//}

		// 세션에서의 Connect / Disconnect 
		// Game에서의 Connect / Disconnect
		// 작업도 분리

		inline void EnqueueSystemJob(SessionConnectionEventPtr& systemJob)
		{
			m_sessionConnEventQueue.Push(systemJob);
		}

		//inline SessionConnectionEventPtr DequeueSystemJob()
		//{
		//	return m_sessionConnEventQueue.Pop();
		//}

		static unsigned WINAPI LogicFunc(LPVOID lparam);
		void LobbyLogic();

		// 로직 스레드의 실행을 종료
		void Stop();

	private:
		ErrorCode ProcessPacket(LONGLONG sessionId, DWORD packetType, PacketPtr& packet)
		{
			if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
				return ErrorCode::CANNOT_FIND_PACKET_FUNC;

			return (this->*m_packetFuncDic[packetType])(sessionId, packet);
		}

		void ProcessNetJob();
		void ProcessSystemJob();

		// 함수 정의
		ErrorCode ProcessEchoPacket(LONGLONG sessionId,PacketPtr& packet);

		std::unordered_map<DWORD, PacketFunc> m_packetFuncDic;
		//std::unique_ptr<jh_content::UserManager> m_pUserManager;

		jh_utility::LockQueue<JobPtr> m_netJobQueue;
		jh_utility::LockQueue<SessionConnectionEventPtr> m_sessionConnEventQueue;

		HANDLE m_hLogicThread;
		jh_network::IocpServer* m_pOwner;
		volatile bool m_bRunningFlag;
	};
}
