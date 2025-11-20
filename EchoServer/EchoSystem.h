#pragma once

namespace jh_content
{
	class EchoSystem
	{
		using PacketFunc = void(EchoSystem::*)(LONGLONG, PacketBufferRef&);

	public:
		EchoSystem(jh_network::IocpServer* owner);
		~EchoSystem();

		void Init();

		inline void EnqueueJob(JobRef& job)
		{
			m_netJobQueue.Push(job);
		}

		// 세션에서의 Connect / Disconnect 
		// Game에서의 Connect / Disconnect
		// 작업도 분리

		inline void EnqueueSystemJob(SessionConnectionEventRef& systemJob)
		{
			m_sessionConnEventQueue.Push(systemJob);
		}

		static unsigned WINAPI LogicFunc(LPVOID lparam);
		void LobbyLogic();

		// 로직 스레드의 실행을 종료
		void Stop();

	private:
		void ProcessPacket(LONGLONG sessionId, DWORD packetType, PacketBufferRef& packet)
		{
			if (m_packetFuncDic.find(packetType) == m_packetFuncDic.end())
				return;

			(this->*m_packetFuncDic[packetType])(sessionId, packet);
		}

		void ProcessNetJob();
		void ProcessSystemJob();

		// 함수 정의
		void ProcessEchoPacket(LONGLONG sessionId,PacketBufferRef& packet);

		std::unordered_map<DWORD, PacketFunc> m_packetFuncDic;

		jh_utility::LockQueue<JobRef> m_netJobQueue;
		jh_utility::LockQueue<SessionConnectionEventRef> m_sessionConnEventQueue;

		HANDLE m_hLogicThread;
		jh_network::IocpServer* m_pOwner;
		volatile bool m_bRunningFlag;
	};
}
