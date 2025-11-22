#pragma once

#include "Room.h"
#include "CMonitor.h"
#include "LobbyDefine.h"

namespace jh_content
{
	class RoomManager
	{
	public:
		RoomManager(USHORT maxRoomCnt, USHORT maxRoomUserCnt);

		~RoomManager();

		RoomPtr CreateRoom(UserPtr userPtr, WCHAR* roomName);
		void DestroyRoom(USHORT roomNum);

		RoomPtr GetRoom(USHORT roomNum);

		std::vector<jh_content::RoomInfo> GetRoomInfos();;

	private:
		USHORT								m_usMaxRoomCnt;
		USHORT								m_usMaxRoomUserCnt; // room당 최대 user 수

		std::unordered_map<USHORT, RoomPtr> m_roomMap;			// [roomNum, roomPtr]
	};
}