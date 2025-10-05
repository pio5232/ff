#pragma once

namespace jh_content
{
	class EchoSystem
	{
		//using PacketFunc = ErrorCode(LobbySystem::*)(ULONGLONG, jh_network::SerializationBufferPtr&);
		// ������ �����ϴ� �ʿ��� SessionPtr�� ��Ƶΰ� ������ ���¿��� Process ȣ�� -> ����ϴ� ���ȿ� ���ŵ��� �ʴ� ���� Ȯ���ϴ�.
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

		// ���ǿ����� Connect / Disconnect 
		// Game������ Connect / Disconnect
		// �۾��� �и�

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

		// ���� �������� ������ ����
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

		// �Լ� ����
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
