#pragma once

namespace jh_content
{
	class RoomManager;
	// 현재는 Room을 모두 만들어놓고 (룸의 고유 번호는 증가되는 형태로) 사용할 것임.
	class Room : public std::enable_shared_from_this<Room>
	{
	public:
		using UnicastFunc = std::function<void(ULONGLONG, PacketPtr&)>;
		//using BroadcastFunc = std::function<void(DWORD, ULONGLONG*, PacketPtr&)>;

		enum class RoomState : byte
		{
			IDLE = 0, // 대기 중인 방.
			RUNNING, // 게임 중인 방.
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

		// 빈 방인지 여부를 반환한다. (empty -> true) 
		bool LeaveRoom(UserPtr userWptr);
		
		RoomInfo GetRoomInfo() const;

		USHORT GetCurUserCnt() const { return static_cast<USHORT>(m_userMap.size()); }
		USHORT GetMaxUserCnt() const { return m_roomInfo.m_usMaxUserCnt; }
		USHORT GetRoomNum() const { return m_roomInfo.m_usRoomNum; }
		ULONGLONG GetOwnerId() const { return m_roomInfo.m_ullOwnerId; }
		void* GetRoomNamePtr() { return m_roomInfo.m_wszRoomName; }
		
		const std::unordered_map<ULONGLONG, std::weak_ptr<jh_content::User>>& GetUserMap() const { return m_userMap; }

		USHORT GetReadyCnt() const { return m_usReadyCnt; }

		// 게임 시작 여부를 반환. true - 게임 실행
		bool UpdateUserReadyStatus(UserPtr userWptr, bool isReady, bool sendOpt);

		static int GetAliveRoomCount() { return aliveRoomCount; }

		void SetUnicastFunc(UnicastFunc unicastFunc) { m_unicastFunc = unicastFunc; }
		//void SetBroadcastFunc(BroadcastFunc broadcastFunc) { m_broadcastFunc = broadcastFunc; }

		void BroadCast(PacketPtr& packet, ULONGLONG excludedId = 0); // 패킷, 방번호, 제외할 UserID (없으면 0)
		void Unicast(PacketPtr& packet, ULONGLONG sessionId);
	private:
		static std::atomic<USHORT> aliveRoomCount;

		RoomInfo m_roomInfo;
		RoomState m_roomState = RoomState::IDLE;
		USHORT m_usReadyCnt;

		std::unordered_map<ULONGLONG, std::weak_ptr<jh_content::User>> m_userMap; // USER ID - USER  

		UnicastFunc m_unicastFunc;
		//BroadcastFunc m_broadcastFunc;
	};
}
