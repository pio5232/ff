#include "pch.h"
#include "Room.h"
#include "NetworkBase.h"
#include "RoomManager.h"
#include "PacketBuilder.h"
#include "User.h"
#include "Memory.h"
//#include "LobbySession.h"

std::atomic<USHORT> jh_content::Room::aliveRoomCount = 0;

jh_content::RoomInfo jh_content::Room::GetRoomInfo() const
{
	jh_content::RoomInfo ri;
	ri.m_ullOwnerId = m_roomInfo.m_ullOwnerId;
	ri.m_usCurUserCnt = static_cast<USHORT>(m_userMap.size());
	ri.m_usMaxUserCnt = m_roomInfo.m_usMaxUserCnt;
	ri.m_usRoomNum = m_roomInfo.m_usRoomNum;

	wcscpy_s(ri.m_wszRoomName, m_roomInfo.m_wszRoomName);

	return ri;
}


jh_content::Room::Room(ULONGLONG ownerId, USHORT maxUserCnt, USHORT roomNum, WCHAR* roomName) : m_roomInfo{ .m_ullOwnerId = ownerId,.m_usRoomNum = roomNum,
.m_usCurUserCnt = 0, .m_usMaxUserCnt = maxUserCnt,.m_wszRoomName{} }, m_usReadyCnt(0), m_sendPacketFunc{}
{
	aliveRoomCount.fetch_add(1);

	wmemcpy_s(m_roomInfo.m_wszRoomName, ROOM_NAME_MAX_LEN, roomName, ROOM_NAME_MAX_LEN);

	m_userMap.reserve(maxUserCnt);

}

jh_content::Room::~Room()
{
	printf("�� ����\n");

	aliveRoomCount.fetch_sub(1);
}

jh_content::Room::RoomEnterResult jh_content::Room::TryEnterRoom(UserPtr userPtr)
{
	if (m_roomInfo.m_usMaxUserCnt <= m_userMap.size())
		return jh_content::Room::RoomEnterResult::FULL;

	else if (m_roomState == RoomState::RUNNING)
		return jh_content::Room::RoomEnterResult::ALREADY_STARTED;
	
	ULONGLONG userId = userPtr->GetUserId();

	m_userMap.insert(std::make_pair(userId, std::weak_ptr<User>(userPtr)));

	userPtr->SetRoom(shared_from_this());

	return jh_content::Room::RoomEnterResult::SUCCESS;
}


bool jh_content::Room::LeaveRoom(UserPtr userPtr)
{
	bool isReady = false;

	ULONGLONG userId = userPtr->GetUserId();

	userPtr->SetRoom(RoomPtr(nullptr));

	m_userMap.erase(userId);
	
	{
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildLeaveRoomNotifyPacket(userId);

		BroadCast(sendBuffer);
	}

	if (m_userMap.size() == 0)
		return true;

	
	// �����µ� ���� �����̶�� ������ �Ѱ��ش�.
	if (userId == m_roomInfo.m_ullOwnerId)
	{
		// ����
		m_roomInfo.m_ullOwnerId = m_userMap.begin()->first;

		std::weak_ptr<User> ownerWptr = m_userMap[m_roomInfo.m_ullOwnerId];
		
		UserPtr newOwnerUser = ownerWptr.lock();

		if (newOwnerUser != nullptr)
		{
			UpdateUserReadyStatus(newOwnerUser, false, false);

			PacketPtr sendBuffer = jh_content::PacketBuilder::BuildOwnerChangeNotifyPacket(newOwnerUser->GetUserId());

			BroadCast(sendBuffer);
		}
	}
	else
	{
		if (isReady)
			m_usReadyCnt--;
	}
	
}



bool jh_content::Room::UpdateUserReadyStatus(UserPtr userPtr, bool isReady, bool sendOpt) // Ready ������ ��ο��� �˸� ���ΰ�. (���� ��ü�� ��� false => �˸��� �ʴ´�.)
{
	if (userPtr->GetReadyState() == isReady)
		return false;

	userPtr->SetReadyState(isReady);
	 
	if (isReady)
		m_usReadyCnt++;
	else
		m_usReadyCnt--;

	ULONGLONG userId = userPtr->GetUserId();

	if (userId == m_roomInfo.m_ullOwnerId && m_usReadyCnt == GetCurUserCnt() && m_roomState == RoomState::IDLE)
	{
		m_roomState = RoomState::RUNNING;

		jh_network::GameStartNotifyPacket startNotifyPacket;

		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildGameStartNotifyPacket();

		BroadCast(sendBuffer);

		return true;
	}

	if (sendOpt)
	{
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildGameReadyNotifyPacket(userId, isReady);

		BroadCast(sendBuffer);
	}

	return false;
}

void jh_content::Room::BroadCast(PacketPtr& packet, ULONGLONG excludedUserId)
{
	for (const auto& [_userId, _userWptr] : m_userMap)
	{
		UserPtr _userPtr = _userWptr.lock();

		if (_userPtr == nullptr)
			continue;

		if (_userId == excludedUserId)
			continue;

		ULONGLONG _sessionId = _userPtr->GetSessionId();

		m_sendPacketFunc(_sessionId, packet);
	}
	
}

void jh_content::Room::Unicast(PacketPtr& packet, ULONGLONG sessionId)
{
	m_sendPacketFunc(sessionId, packet);
}
