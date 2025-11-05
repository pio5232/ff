#include "pch.h"
#include "PacketBuilder.h"
#include "Memory.h"
#include "SerializationBuffer.h"

PacketPtr jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode errorCode)
{
	jh_network::ErrorPacket errorPacket;

	errorPacket.packetErrorCode = errorCode;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(errorPacket));

	*buffer << errorPacket.size << errorPacket.type << errorPacket.packetErrorCode;

	return buffer;

}

PacketPtr jh_content::PacketBuilder::BuildLogInResponsePacket(ULONGLONG userId)
{
	jh_network::LogInResponsePacket logInResponsePacket; 

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(logInResponsePacket));

	*buffer << logInResponsePacket.size << logInResponsePacket.type << userId;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildMakeRoomResponsePacket(bool isMade, ULONGLONG ownerId, USHORT roomNum, USHORT curUserCnt, USHORT maxUserCnt, const WCHAR* roomName)
{
	jh_network::MakeRoomResponsePacket makeRoomResponsePacket;

	makeRoomResponsePacket.isMade = true;

	makeRoomResponsePacket.roomInfo.m_ullOwnerId = ownerId;
	makeRoomResponsePacket.roomInfo.m_usRoomNum = roomNum;
	makeRoomResponsePacket.roomInfo.m_usCurUserCnt = curUserCnt;
	makeRoomResponsePacket.roomInfo.m_usMaxUserCnt = maxUserCnt;
	wmemcpy_s(makeRoomResponsePacket.roomInfo.m_wszRoomName, ROOM_NAME_MAX_LEN, roomName, ROOM_NAME_MAX_LEN);

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(makeRoomResponsePacket));

	*buffer << makeRoomResponsePacket.size << makeRoomResponsePacket.type << makeRoomResponsePacket.isMade << makeRoomResponsePacket.roomInfo;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildEnterRoomNotifyPacket(ULONGLONG userId)
{
	jh_network::EnterRoomNotifyPacket enterRoomNotifyPacket;

	enterRoomNotifyPacket.enterUserId = userId;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(enterRoomNotifyPacket));

	*buffer << enterRoomNotifyPacket.size << enterRoomNotifyPacket.type << enterRoomNotifyPacket.enterUserId;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildEnterRoomResponsePacket(bool isAllowed, const std::vector<ULONGLONG>& userIdAndReadyList)
{
	jh_network::EnterRoomResponsePacket enterRoomResponsePacket;

	enterRoomResponsePacket.bAllow = isAllowed;
	enterRoomResponsePacket.idCnt = userIdAndReadyList.size();

	enterRoomResponsePacket.size = sizeof(bool) + sizeof(USHORT) + sizeof(ULONGLONG) * userIdAndReadyList.size();
	
	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(enterRoomResponsePacket) + enterRoomResponsePacket.size);

	*buffer << enterRoomResponsePacket.size << enterRoomResponsePacket.type << enterRoomResponsePacket.bAllow << enterRoomResponsePacket.idCnt;

	for (ULONGLONG userId : userIdAndReadyList)
	{
		*buffer << userId;
	}

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildLeaveRoomNotifyPacket(ULONGLONG userId)
{
	jh_network::LeaveRoomNotifyPacket leaveRoomNotifyPacket;

	leaveRoomNotifyPacket.leaveUserId = userId;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(leaveRoomNotifyPacket));

	*buffer << leaveRoomNotifyPacket.size << leaveRoomNotifyPacket.type << leaveRoomNotifyPacket.leaveUserId;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildLeaveRoomResponsePacket()
{
	jh_network::LeaveRoomResponsePacket leaveRoomResPacket;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(leaveRoomResPacket));

	*buffer << leaveRoomResPacket.size << leaveRoomResPacket.type;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildRoomListResponsePacket(std::vector<jh_content::RoomInfo>& roomInfos)
{
	jh_network::PacketHeader header;

	USHORT roomCnt = roomInfos.size();
	header.size = sizeof(roomCnt) + roomCnt * jh_content::RoomInfo::GetSize();
	header.type = jh_network::ROOM_LIST_RESPONSE_PACKET;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(jh_network::RoomListResponsePacket) + roomCnt * jh_content::RoomInfo::GetSize());

	*buffer << header.size << header.type << roomCnt;

	for (const jh_content::RoomInfo& roomInfo : roomInfos)
	{
		*buffer << roomInfo.m_ullOwnerId << roomInfo.m_usRoomNum << roomInfo.m_usCurUserCnt << roomInfo.m_usMaxUserCnt;

		buffer->PutData(reinterpret_cast<const char*>(roomInfo.m_wszRoomName), ROOM_NAME_MAX_LEN * MESSAGE_SIZE);
	}

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildOwnerChangeNotifyPacket(ULONGLONG newOwnerUserId)
{
	jh_network::OwnerChangeNotifyPacket ownerChangeNotifyPacket;
	ownerChangeNotifyPacket.userId = newOwnerUserId;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(ownerChangeNotifyPacket));

	*buffer << ownerChangeNotifyPacket.size << ownerChangeNotifyPacket.type << ownerChangeNotifyPacket.userId;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameStartNotifyPacket()
{
	jh_network::GameStartNotifyPacket gameStartNotifyPacket;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(gameStartNotifyPacket));

	*buffer << gameStartNotifyPacket.size << gameStartNotifyPacket.type;
	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameReadyNotifyPacket(ULONGLONG userId, bool isReady)
{
	jh_network::GameReadyNotifyPacket gameReadyNotifyPacket;

	gameReadyNotifyPacket.userId = userId;
	gameReadyNotifyPacket.isReady = isReady;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(gameReadyNotifyPacket));

	*buffer << gameReadyNotifyPacket.size << gameReadyNotifyPacket.type << gameReadyNotifyPacket.isReady << gameReadyNotifyPacket.userId;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildEchoPacket(ULONGLONG data)
{
	jh_network::EchoPacket echoPacket;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(echoPacket));

	*buffer << echoPacket.size << echoPacket.type << echoPacket.m_data;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildLanInfoPacket(WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG tok)
{
	jh_network::GameServerLanInfoPacket gameServerLanInfoPacket;
	
	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(gameServerLanInfoPacket));
	
	*buffer << gameServerLanInfoPacket.size << gameServerLanInfoPacket.type;

	buffer->PutData(reinterpret_cast<const char*>(ipStr), IP_STRING_LEN * MESSAGE_SIZE);
	
	*buffer << port << roomNum << tok;


	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameServerSettingResponsePacket(USHORT roomNum, USHORT requiredUserCnt, USHORT maxUserCnt)
{
	jh_network::GameServerSettingResponsePacket gameServerSettingResponsePacket;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(gameServerSettingResponsePacket));

	*buffer << gameServerSettingResponsePacket.size << gameServerSettingResponsePacket.type << roomNum << requiredUserCnt << maxUserCnt;

	return buffer;
}


