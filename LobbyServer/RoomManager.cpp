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

std::vector<jh_content::RoomInfo> jh_content::RoomManager::GetRoomInfos()
{
	std::vector<jh_content::RoomInfo> v;

	for (const auto& [roomNum, roomPtr] : m_roomMap)
	{
		v.emplace_back(roomPtr->GetRoomInfo());
	}

	return v;
}

void jh_content::RoomManager::Init()
{
	//jh_utility::Parser parser;

	//parser.LoadFile(UP_DIR(LOBBY_DATA_CONFIG_FILE));
	//parser.SetReadingCategory(LOBBY_DATA_CATEGORY_NAME);

	//bool succeeded = parser.GetValue(L"maxRoomCount", m_usMaxRoomCnt);
	//succeeded &= parser.GetValue(L"maxRoomUserCount", m_usMaxRoomUserCnt);

	//parser.CloseFile();

	//if (true == succeeded)
	//	_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"LobbyServer Parse Complete [FileName : %s]", LOBBY_DATA_CONFIG_FILE);
	//else
	//{
	//	_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"LobbyServer Parse Get Failed Error");

	//	jh_utility::CrashDump::Crash();
	//}

	//m_roomMap.reserve(m_usMaxRoomCnt);
}
//
//ErrorCode jh_content::RoomManager::SendToUserRoomInfo(LobbySessionPtr lobbySessionPtr)
//{
//	std::vector<std::weak_ptr<Room>> lazyProcBuf;
//
//	{
//		SRWLockGuard lockGuard(&_lock);
//
//		for (auto& roomPair : _roomMap)
//		{
//			lazyProcBuf.push_back(roomPair.second);
//		}
//	}
//
//	uint16 roomCnt = lazyProcBuf.size();
//
//	PacketHeader header;
//	header.size = sizeof(roomCnt) + roomCnt * RoomInfo::GetSize();;
//	header.type = ROOM_LIST_RESPONSE_PACKET;
//
//	jh_network::SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(RoomListResponsePacket) + roomCnt * sizeof(RoomInfo));
//
//	*sendBuffer << header << roomCnt;
//
//	for (std::weak_ptr<Room>& weakPtrRoom : lazyProcBuf)
//	{
//		if (false == weakPtrRoom.expired())
//		{
//			RoomPtr sharedRoom = weakPtrRoom.lock();
//
//			*sendBuffer << sharedRoom->GetOwnerId() << sharedRoom->GetRoomNum() << sharedRoom->GetCurUserCnt() << sharedRoom->GetMaxUserCnt();
//			sendBuffer->PutData(static_cast<const char*>(sharedRoom->GetRoomNamePtr()), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);
//		}
//	}
//
//	lobbySessionPtr->Send(sendBuffer);
//
//	return ErrorCode::NONE;
//}

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
		_LOG(L"RoomManager", LOG_LEVEL_WARNING, L"[DestroyRoom] 요청한 방이 존재하지 않음 [%us]", roomNum);

		return;
	}
}
