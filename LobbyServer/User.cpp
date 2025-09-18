#include "pch.h"
#include "User.h"
#include "Room.h"

alignas(64) ULONGLONG jh_content::User::m_ullAliveLobbyUserCount = 0;


std::pair<bool, USHORT> jh_content::User::TryGetRoomId() const
{
	return { m_bJoinFlag, m_usRoomNum };
}

void jh_content::User::SetRoom(RoomPtr roomPtr)
{
	if (roomPtr != nullptr)
	{
		m_usRoomNum = roomPtr->GetRoomNum();
		
		SetReadyState(false);

		m_bJoinFlag = true;
	}
	else
	{
		m_usRoomNum = 0;

		m_bJoinFlag = false;
	}
	m_myRoom = roomPtr;
}
