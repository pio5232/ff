#pragma once

namespace jh_content
{
	class DummyUpdateSystem
	{
	public:
		using PacketFunc = void(DummyUpdateSystem::*)(ULONGLONG, PacketPtr&, int);
		
		struct EtcData
		{
			LONG m_llDestroyedRoomErrorCount = 0;
			LONG m_llDiffRoomNameErrorCount = 0;
			LONG m_llFullRoomErrorCount = 0;
			LONG m_llAlreadyRunningRoomErrorCount = 0;
			LONG m_lReSendTimeoutCount = 0;
		};
	private:

		struct alignas(64) LogicData
		{
		
			struct ErrorAggregator
			{
				alignas(32) LONG m_llDestroyedRoom = 0;
				alignas(32) LONG m_llDiffRoomName = 0;
				alignas(32) LONG m_llFullRoom = 0;
				alignas(32) LONG m_llAlreadyRunningRoom = 0;
			};
		public:
			LogicData() : m_hLogicThread{}, m_hJobEvent{}, m_netJobQueue{}, m_sessionConnEventQueue{}, m_dummyUmap{}, m_reSendTimeoutCnt{} {}

			ErrorAggregator m_errorAggregator;
			alignas(32) LONG m_reSendTimeoutCnt;

			HANDLE m_hLogicThread;
			HANDLE m_hJobEvent;
			jh_utility::LockQueue<JobPtr> m_netJobQueue;
			jh_utility::LockQueue<SessionConnectionEventPtr> m_sessionConnEventQueue;
			std::unordered_map<ULONGLONG, DummyPtr> m_dummyUmap;
			std::unordered_map<DummyPtr, int> m_dummyVectorIndexUMap;
			std::vector<DummyPtr> m_dummyVector;
			
			
		};

		struct WorkerTransData
		{
			void* m_pThis;
			int m_iThreadNum;
		};
	
	private:
		void ProcessPacket(ULONGLONG sessionId, DWORD packetType, PacketPtr& packet, int threadNum);
	
	public:
		static unsigned WINAPI LogicThreadFunc(LPVOID lparam);
		void DummyLogic(int threadNum);

		DummyUpdateSystem(jh_network::IocpClient* owner);
		~DummyUpdateSystem();

		void Init();
		void Stop();

		void EnqueueJob(JobPtr& job, int idx);
		void EnqueueSessionConnEvent(SessionConnectionEventPtr& connectionEvent, int idx);
		void ProcessNetJob(int threadNum);
		void ProcessSessionConnectionEvent(int threadNum);

		void ProcessDummyLogic(int threadNum);

		void SendToDummy(DummyPtr& dummy, PacketPtr& packet, DummyStatus nextStauts = DummyStatus::NO_CHANGE);

		void HandleRoomListResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleLogInResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleMakeRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleEnterRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleChatNotifyPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleChatResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleLeaveRoomResponsePacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		void HandleEchoPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);

		void HandleErrorPacket(ULONGLONG sessionId, PacketPtr& packet, int threadNum);
		bool IsValidThreadNum(int threadNum) { return threadNum >= 0 && threadNum < LOGIC_THREAD_COUNT; }


		EtcData UpdateEtc();
		void CheckSendTimeOut(int threadNum);

		ULONGLONG GetRTT() const;

		std::unordered_map<USHORT, PacketFunc> m_packetFuncDic;
		jh_network::IocpClient* m_pOwner;

		LogicData m_logicData[LOGIC_THREAD_COUNT];
		
		alignas(64) volatile char m_bRunnigFlag;
		alignas(64) ULONGLONG m_ullRtt;





	public:
		alignas(64) std::atomic<bool> bbbbb = false;
	};
}

