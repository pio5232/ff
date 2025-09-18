#pragma once

namespace jh_content
{
	class Room;
	class User
	{

	public:

		User(ULONGLONG sessionId, ULONGLONG userId) : m_ullUserId(userId), m_ullSessionId(sessionId), m_bIsReady(false), m_bJoinFlag(false),
			m_usRoomNum(0)
		{
			InterlockedIncrement64((LONGLONG*)&m_ullAliveLobbyUserCount);
		}

		~User()
		{
			InterlockedDecrement64((LONGLONG*)&m_ullAliveLobbyUserCount);
		}


		// Resulta
		bool GetReadyState() const { return m_bIsReady; }

		// true - 준비 상태
		void SetReadyState(bool isReady) { m_bIsReady = isReady; }

		std::pair<bool, USHORT> TryGetRoomId() const;
		ULONGLONG GetUserId() const { return m_ullUserId; }
		ULONGLONG GetSessionId() const { return m_ullSessionId; }
		
		void SetRoom(RoomPtr roomPtr);
	public:
		static alignas(64) ULONGLONG m_ullAliveLobbyUserCount;
	private:
		const ULONGLONG m_ullSessionId;
		ULONGLONG m_ullUserId;
		bool m_bIsReady;

		USHORT m_usRoomNum;
		bool m_bJoinFlag;

		std::weak_ptr<Room> m_myRoom;
		// 나갈땐 invalid로 설정.
	};
}
