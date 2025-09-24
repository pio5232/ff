#pragma once

namespace jh_content
{
	class RoomManager;
	// ����� Room�� ��� �������� (���� ���� ��ȣ�� �����Ǵ� ���·�) ����� ����.
	class Room : public std::enable_shared_from_this<Room>
	{
	public:
		using SendPacketFunc = std::function<void(ULONGLONG, PacketPtr&)>;

		enum class RoomState : byte
		{
			IDLE = 0, // ��� ���� ��.
			RUNNING, // ���� ���� ��.
		};

		enum class RoomEnterResult : byte
		{
			SUCCESS = 0,

			FULL,
			ALREADY_STARTED,
		};
		Room(ULONGLONG ownerId, USHORT maxUserCnt, USHORT roomNum, WCHAR* roomName);
		~Room();

		RoomEnterResult TryEnterRoom(UserPtr userWptr);

		// �� ������ ���θ� ��ȯ�Ѵ�. (empty -> true) 
		bool LeaveRoom(UserPtr userWptr);
		
		RoomInfo GetRoomInfo() const;

		USHORT GetCurUserCnt() const { return static_cast<USHORT>(m_userMap.size()); }
		USHORT GetMaxUserCnt() const { return m_roomInfo.m_usMaxUserCnt; }
		USHORT GetRoomNum() const { return m_roomInfo.m_usRoomNum; }
		ULONGLONG GetOwnerId() const { return m_roomInfo.m_ullOwnerId; }
		void* GetRoomNamePtr() { return m_roomInfo.m_wszRoomName; }
		
		const std::unordered_map<ULONGLONG, std::weak_ptr<jh_content::User>>& GetUserMap() const { return m_userMap; }

		USHORT GetReadyCnt() const { return m_usReadyCnt; }

		// ���� ���� ���θ� ��ȯ. true - ���� ����
		bool UpdateUserReadyStatus(UserPtr userWptr, bool isReady, bool sendOpt);

		static int GetAliveRoomCount() { return aliveRoomCount; }

		void SetSendPacketFunc(SendPacketFunc sendPacketFunc) { m_sendPacketFunc = sendPacketFunc; }

		void BroadCast(PacketPtr& packet, ULONGLONG excludedId = 0); // ��Ŷ, ���ȣ, ������ UserID (������ 0)
		void Unicast(PacketPtr& packet, ULONGLONG sessionId);
	private:
		static std::atomic<USHORT> aliveRoomCount;

		RoomInfo m_roomInfo;
		RoomState m_roomState = RoomState::IDLE;
		USHORT m_usReadyCnt;

		std::unordered_map<ULONGLONG, std::weak_ptr<jh_content::User>> m_userMap; // USER ID - USER  

		SendPacketFunc m_sendPacketFunc;
		
		//ULONGLONG m_ullOwnerId = 0; // == userId;
		//const USHORT m_usMaxUserCnt = 0;
		//USHORT m_usRoomNumber = 0; // �� ��° ������ Ȯ���Ѵ�.. ����ڰ� CreateRoom�ϸ� Room��ȣ�� �þ���� ������.
		//WCHAR m_wszRoomName[ROOM_NAME_MAX_LEN] = {};


	};
}
