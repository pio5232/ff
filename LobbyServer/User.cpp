#include "pch.h"
#include "User.h"
#include "Room.h"

alignas(32) volatile LONG jh_content::User::aliveLobbyUserCount = 0;

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
