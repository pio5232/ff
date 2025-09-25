#pragma once

#include "Room.h"
#include "CMonitor.h"
#include "LobbyDefine.h"

namespace jh_content
{
	// ������ ������ �ϳ����ٵ�.

	class RoomManager
	{
	public:
		RoomManager(USHORT maxRoomCnt, USHORT maxRoomUserCnt);

		~RoomManager();

		void Init();
		//ErrorCode SendToUserRoomInfo(LobbySessionPtr lobbySessionPtr);

		// CreateRoom -> User�� ���������� �� �����.
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
		USHORT m_usMaxRoomUserCnt; // room�� �ִ� user ��

		// room�� �������� vector�� ���� �׳�.??
		std::unordered_map<USHORT, RoomPtr> m_roomMap; // [roomNum, roomPtr]
	};
}