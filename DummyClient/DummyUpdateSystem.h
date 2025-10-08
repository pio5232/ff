#pragma once

namespace jh_content
{
	class DummyUpdateSystem
	{
	public:
		using PacketFunc = void(DummyUpdateSystem::*)(ULONGLONG, PacketPtr&);
		
	private:
		struct alignas(64) LogicData
		{
			LogicData() : m_hLogicThread{}, m_hJobEvent{}, m_netJobQueue{} {}

			void ProcessNetJob()
			{
				
			}

			HANDLE m_hLogicThread;
			HANDLE m_hJobEvent;
			jh_utility::LockQueue<JobPtr> m_netJobQueue;
		};

		struct WorkerTransData
		{
			void* m_pThis;
			int m_iThreadNum;
		};
	
	private:
		void ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet);
	
	public:
		static unsigned WINAPI Worker(LPVOID lparam);
		void WorkerThread(int threadNum);

		DummyUpdateSystem(jh_network::IocpClient* owner);
		~DummyUpdateSystem();

		void Init();
		void Stop();

		void EnqueueJob(JobPtr& job, int idx);

		void HandleRoomListResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleLogInResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleMakeRoomResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleEnterRoomResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleChatNotifyPacket(ULONGLONG session, PacketPtr& packet);
		void HandleLeaveRoomResponsePacket(ULONGLONG session, PacketPtr& packet);

		std::unordered_map<USHORT, PacketFunc> m_packetFuncDic;
		jh_network::IocpClient* m_pOwner;

		LogicData m_logicData[LOGIC_THREAD_COUNT];
		alignas(64) std::atomic<bool> m_bRunnigFlag;
	};
}

