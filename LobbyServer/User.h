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
			InterlockedIncrement(&aliveLobbyUserCount);
		}

		~User()
		{
			InterlockedDecrement(&aliveLobbyUserCount);
		}

		void SetReadyState(bool isReady)				{ m_bIsReady = isReady; }
		void SetRoom(RoomPtr roomPtr);

		std::pair<bool, USHORT> TryGetRoomId() const { return { m_bJoinFlag, m_usRoomNum }; }
		
		bool GetReadyState() const						{ return m_bIsReady; }
		ULONGLONG GetUserId() const						{ return m_ullUserId; }
		ULONGLONG GetSessionId() const					{ return m_ullSessionId; }
	public:
		static alignas(32) volatile LONG	aliveLobbyUserCount;
	private:
		const ULONGLONG						m_ullSessionId;
		ULONGLONG							m_ullUserId;
		bool								m_bIsReady;

		USHORT								m_usRoomNum;
		bool								m_bJoinFlag;

		std::weak_ptr<Room>					m_myRoom;
	};
}
