#include "pch.h"
#include "User.h"

alignas(64) volatile ULONGLONG jh_content::User::_aliveLobbyUserCount = 0;
const std::pair<bool, USHORT> jh_content::User::GetRoomId() const
{
	return { _isJoinedRoom, _roomNum };
}
