#pragma once

namespace jh_content
{
	class DummyUpdateSystem
	{
		struct LogicData
		{
			LogicData() : m_hLogicThread{}, m_hJobEvent{}, m_netJobQueue{} {}

			HANDLE m_hLogicThread;
			HANDLE m_hJobEvent;
			jh_utility::LockQueue<JobPtr> m_netJobQueue;
		};
	public:
		using PacketFunc = void(DummyUpdateSystem::*)(ULONGLONG, PacketPtr&);
		
		DummyUpdateSystem();
		~DummyUpdateSystem();

		void HandleRoomListResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleLogInResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleMakeRoomResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleEnterRoomResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleChatNotifyPacket(ULONGLONG session, PacketPtr& packet);
		void HandleLeaveRoomResponsePacket(ULONGLONG session, PacketPtr& packet);

		const std::unordered_map<USHORT, PacketFunc>& m_packetFuncDic;

		LogicData m_logicData[LOGIC_THREAD_COUNT];

	};
}

