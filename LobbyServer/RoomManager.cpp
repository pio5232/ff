#include "pch.h"
#include "RoomManager.h"
#include "NetworkBase.h"
#include "User.h"
#include "Memory.h"

jh_content::RoomManager::RoomManager(USHORT maxRoomCnt, USHORT maxRoomUserCnt) : m_usMaxRoomCnt(maxRoomCnt), m_usMaxRoomUserCnt(maxRoomUserCnt)
{
	m_roomMap.reserve(m_usMaxRoomCnt);
}

jh_content::RoomManager::~RoomManager()
{
}

RoomPtr jh_content::RoomManager::GetRoom(USHORT roomNum)
{
	auto iter = m_roomMap.find(roomNum);
	if (iter == m_roomMap.end())
	{
		_LOG(ROOM_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetRoom] Room not found. Room : [%hu].", roomNum);
		return nullptr;
	}

	return iter->second;
}

std::vector<jh_content::RoomInfo> jh_content::RoomManager::GetRoomInfos()
{
	std::vector<jh_content::RoomInfo> v;

	for (const auto& [roomNum, roomPtr] : m_roomMap)
	{
		v.emplace_back(roomPtr->GetRoomInfo());
	}

	return v;
}

RoomPtr jh_content::RoomManager::CreateRoom(UserPtr userPtr, WCHAR* roomName)
{
	if (m_roomMap.size() == m_usMaxRoomCnt)
		return nullptr;

	static volatile USHORT roomNumGen = UINT16_MAX;

	roomNumGen = (roomNumGen % UINT16_MAX) + 1;

	RoomPtr roomPtr = MakeShared<jh_content::Room>(g_memSystem, userPtr->GetUserId(), m_usMaxRoomUserCnt, roomNumGen, roomName);

	m_roomMap.insert({ roomNumGen, roomPtr });

	return roomPtr;
}

void jh_content::RoomManager::DestroyRoom(USHORT roomNum)
{
	size_t delSize = m_roomMap.erase(roomNum);
	
	if (0 == delSize)
	{
		_LOG(L"RoomManager", LOG_LEVEL_WARNING, L"[DestroyRoom] Room not found: [%hu]", roomNum);

		return;
	}
}
