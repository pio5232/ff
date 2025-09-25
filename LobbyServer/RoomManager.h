#pragma once

#include "Room.h"
#include "CMonitor.h"
#include "LobbyDefine.h"

namespace jh_content
{
	// 어차피 로직은 하나일텐데.

	class RoomManager
	{
	public:
		RoomManager(USHORT maxRoomCnt, USHORT maxRoomUserCnt);

		~RoomManager();

		void Init();
		//ErrorCode SendToUserRoomInfo(LobbySessionPtr lobbySessionPtr);

		// CreateRoom -> User가 접속해있을 때 만든다.
		RoomPtr CreateRoom(UserPtr userPtr, WCHAR* roomName);
		void DestroyRoom(USHORT roomNum);


		RoomPtr GetRoom(USHORT roomNum)
		{
			auto iter = m_roomMap.find(roomNum);
			if (iter == m_roomMap.end())
			{
				_LOG(ROOM_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"Request Room %hu is not exist", roomNum);
				return nullptr;
			}

			return iter->second;
		}

		std::vector<jh_content::RoomInfo> GetRoomInfos();;

	private:
		USHORT m_usMaxRoomCnt;
		USHORT m_usMaxRoomUserCnt; // room당 최대 user 수

		// room을 만들어놓고 vector로 쓰자 그냥.??
		std::unordered_map<USHORT, RoomPtr> m_roomMap; // [roomNum, roomPtr]
	};
}