#include "pch.h"
#include "PacketBuilder.h"
#include "Memory.h"

PacketPtr jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode errorCode)
{
	jh_network::ErrorPacket errorPacket;

	errorPacket.packetErrorCode = errorCode;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(errorPacket));

	*buffer << errorPacket.size << errorPacket.type << errorPacket.packetErrorCode;

	return buffer;
}

PacketPtr jh_content::PacketBuilder::BuildAttackNotifyPacket(ULONGLONG entityId)
{
	jh_network::AttackNotifyPacket attackNotifyPacket;
	//attackNotifyPacket.entityId = entityId;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(attackNotifyPacket));

	*sendBuffer << attackNotifyPacket.size << attackNotifyPacket.type << entityId;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildAttackedNotifyPacket(ULONGLONG entityId, USHORT currentHp)
{
	jh_network::AttackedNotifyPacket attackedNotifyPacket;
	//attackedNotifyPacket.entityId = entityId;
	//attackedNotifyPacket.currentHp = currentHp;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(attackedNotifyPacket));

	*sendBuffer << attackedNotifyPacket.size << attackedNotifyPacket.type << entityId << currentHp;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMakeMyCharacterPacket(ULONGLONG entityId, const Vector3& position)
{
	jh_network::MakeMyCharacterPacket makeMyCharacterPacket;

	//makeMyCharacterPacket.entityId = entityId;
	//makeMyCharacterPacket.pos = position;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(makeMyCharacterPacket));

	*sendBuffer << makeMyCharacterPacket.size << makeMyCharacterPacket.type << entityId << position;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(ULONGLONG entityId, const Vector3& position)
{
	jh_network::MakeOtherCharacterPacket makeOtherCharacterPacket;

	//makeOtherCharacterPacket.entityId = entityId;
	//makeOtherCharacterPacket.pos = position;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(makeOtherCharacterPacket));

	*sendBuffer << makeOtherCharacterPacket.size << makeOtherCharacterPacket.type << entityId << position;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameInitDonePacket()
{
	jh_network::GameInitDonePacket donePacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(donePacket));

	*sendBuffer << donePacket.size << donePacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(ULONGLONG entityId)
{
	jh_network::DeleteOtherCharacterPacket deleteOtherCharacterPacket;
	//deleteOtherCharacterPacket.entityId = entityId;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(deleteOtherCharacterPacket));

	*sendBuffer << deleteOtherCharacterPacket.size << deleteOtherCharacterPacket.type << entityId;
	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMoveStartNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY)
{
	jh_network::MoveStartNotifyPacket moveStartNotifyPacket;
	//moveStartNotifyPacket.entityId = entityId;
	//moveStartNotifyPacket.pos = pos;
	//moveStartNotifyPacket.rotY = rotY;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(moveStartNotifyPacket));

	*sendBuffer << moveStartNotifyPacket.size << moveStartNotifyPacket.type << entityId << pos << rotY;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMoveStopNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY)
{
	jh_network::MoveStopNotifyPacket moveStopNotifyPacket;
	moveStopNotifyPacket.entityId = entityId;
	moveStopNotifyPacket.pos = pos;
	moveStopNotifyPacket.rotY = rotY;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(moveStopNotifyPacket));

	*sendBuffer << moveStopNotifyPacket.size << moveStopNotifyPacket.type << moveStopNotifyPacket.entityId
		<< moveStopNotifyPacket.pos << moveStopNotifyPacket.rotY;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildUpdateTransformPacket(ULONGLONG timeStamp, ULONGLONG entityId, const Vector3& pos, const Vector3& rot)
{
	jh_network::UpdateTransformPacket updatePacket;

	//updatePacket.timeStamp = timeStamp;
	//updatePacket.entityId = entityId;
	//updatePacket.pos = pos;
	//updatePacket.rot = rot;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(updatePacket));

	*sendBuffer << updatePacket.size << updatePacket.type << timeStamp << entityId << pos << rot;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildDieNotifyPacket(ULONGLONG entityId)
{
	jh_network::DieNotifyPacket dieNotifyPacket;
	//dieNotifyPacket.entityId = entityId;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(dieNotifyPacket));

	*sendBuffer << dieNotifyPacket.size << dieNotifyPacket.type << entityId;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildSpectatorInitPacket()
{
	jh_network::SpectatorInitPacket spectatorInitPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(spectatorInitPacket));

	*sendBuffer << spectatorInitPacket.size << spectatorInitPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildEnterGameResponsePacket()
{
	jh_network::EnterGameResponsePacket enterGameResponsePacket;

	PacketPtr buffer = MakeSharedBuffer(g_memSystem, sizeof(enterGameResponsePacket));

	*buffer << enterGameResponsePacket.size << enterGameResponsePacket.type;

	return buffer;
}


PacketPtr jh_content::PacketBuilder::BuildGameEndNotifyPacket()
{
	jh_network::GameEndNotifyPacket gameEndNotifyPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(gameEndNotifyPacket));

	*sendBuffer << gameEndNotifyPacket.size << gameEndNotifyPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildUpdateWinnerNotifyPacket(ULONGLONG userId, ULONGLONG expectedTimeStamp)
{
	jh_network::UpdateWinnerNotifyPacket updateWinnerNotifyPacket;

	//updateWinnerNotifyPacket.userId = userId;
	//updateWinnerNotifyPacket.expectedTimeStamp = expectedTimeStamp;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(updateWinnerNotifyPacket));

	*sendBuffer << updateWinnerNotifyPacket.size << updateWinnerNotifyPacket.type << userId << expectedTimeStamp;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildInvalidateWinnerNotifyPacket(ULONGLONG canceledUserID)
{
	jh_network::InvalidateWinnerNotifyPacket invalidateWinnerNotifyPacket;
	invalidateWinnerNotifyPacket.canceledUserId = canceledUserID;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(invalidateWinnerNotifyPacket));

	*sendBuffer << invalidateWinnerNotifyPacket.size << invalidateWinnerNotifyPacket.type << invalidateWinnerNotifyPacket.canceledUserId;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildWinnerInfoNotifyPacket()
{
	jh_network::WinnerInfoNotifyPacket winnerInfoNotifyPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(winnerInfoNotifyPacket));

	*sendBuffer << winnerInfoNotifyPacket.size << winnerInfoNotifyPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildCharacterSyncPacket(ULONGLONG entityId, const Vector3& syncPos, const Vector3& syncRot)
{
	jh_network::CharacterPositionSyncPacket syncPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(syncPacket));

	*sendBuffer << syncPacket.size << syncPacket.type << entityId << syncPos << syncRot;
	
	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameStartNotifyPacket()
{
	jh_network::GameStartNotifyPacket gameStartNotifyPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(gameStartNotifyPacket));

	*sendBuffer << gameStartNotifyPacket.size << gameStartNotifyPacket.type;
	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameServerSettingRequestPacket()
{
	jh_network::GameServerSettingRequestPacket settingRequestPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(settingRequestPacket));

	*sendBuffer << settingRequestPacket.size << settingRequestPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameServerLanInfoPacket(const WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG token)
{
	jh_network::GameServerLanInfoPacket gameServerLanInfoPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(gameServerLanInfoPacket));

	wcscpy_s(gameServerLanInfoPacket.ipStr, IP_STRING_LEN, ipStr);

	*sendBuffer << gameServerLanInfoPacket.size << gameServerLanInfoPacket.type;

	sendBuffer->PutData(reinterpret_cast<const char*>(gameServerLanInfoPacket.ipStr), IP_STRING_LEN * MESSAGE_SIZE);

	*sendBuffer << port << roomNum << token;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildChatNotifyPacket(ULONGLONG userId, USHORT messageLen, const char* message)
{
	jh_network::ChatOtherUserNotifyPacket chatNotifyPacket;

	chatNotifyPacket.size = sizeof(userId) + sizeof(messageLen) + messageLen;
	chatNotifyPacket.type = jh_network::CHAT_NOTIFY_PACKET;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memSystem, sizeof(jh_network::PacketHeader) + chatNotifyPacket.size);
	*sendBuffer << chatNotifyPacket.size << chatNotifyPacket.type << userId << messageLen;

	sendBuffer->PutData(message, messageLen);

	return sendBuffer;
}
