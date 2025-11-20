#include "pch.h"
#include "PacketBuilder.h"
#include "Memory.h"

PacketBufferRef jh_content::PacketBuilder::BuildErrorPacket(jh_network::PacketErrorCode errorCode)
{
	jh_network::ErrorPacket errorPacket;

	errorPacket.packetErrorCode = errorCode;

	PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(errorPacket));

	*buffer << errorPacket.size << errorPacket.type << errorPacket.packetErrorCode;

	return buffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildAttackNotifyPacket(ULONGLONG entityId)
{
	jh_network::AttackNotifyPacket attackNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(attackNotifyPacket));

	*sendBuffer << attackNotifyPacket.size << attackNotifyPacket.type << entityId;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildAttackedNotifyPacket(ULONGLONG entityId, USHORT currentHp)
{
	jh_network::AttackedNotifyPacket attackedNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(attackedNotifyPacket));

	*sendBuffer << attackedNotifyPacket.size << attackedNotifyPacket.type << entityId << currentHp;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildMakeMyCharacterPacket(ULONGLONG entityId, const Vector3& position)
{
	jh_network::MakeMyCharacterPacket makeMyCharacterPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(makeMyCharacterPacket));

	*sendBuffer << makeMyCharacterPacket.size << makeMyCharacterPacket.type << entityId << position;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(ULONGLONG entityId, const Vector3& position)
{
	jh_network::MakeOtherCharacterPacket makeOtherCharacterPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(makeOtherCharacterPacket));

	*sendBuffer << makeOtherCharacterPacket.size << makeOtherCharacterPacket.type << entityId << position;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildGameInitDonePacket()
{
	jh_network::GameInitDonePacket donePacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(donePacket));

	*sendBuffer << donePacket.size << donePacket.type;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(ULONGLONG entityId)
{
	jh_network::DeleteOtherCharacterPacket deleteOtherCharacterPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(deleteOtherCharacterPacket));

	*sendBuffer << deleteOtherCharacterPacket.size << deleteOtherCharacterPacket.type << entityId;
	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildMoveStartNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY)
{
	jh_network::MoveStartNotifyPacket moveStartNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(moveStartNotifyPacket));

	*sendBuffer << moveStartNotifyPacket.size << moveStartNotifyPacket.type << entityId << pos << rotY;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildMoveStopNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY)
{
	jh_network::MoveStopNotifyPacket moveStopNotifyPacket;
	moveStopNotifyPacket.entityId = entityId;
	moveStopNotifyPacket.pos = pos;
	moveStopNotifyPacket.rotY = rotY;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(moveStopNotifyPacket));

	*sendBuffer << moveStopNotifyPacket.size << moveStopNotifyPacket.type << moveStopNotifyPacket.entityId
		<< moveStopNotifyPacket.pos << moveStopNotifyPacket.rotY;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildUpdateTransformPacket(ULONGLONG timeStamp, ULONGLONG entityId, const Vector3& pos, const Vector3& rot)
{
	jh_network::UpdateTransformPacket updatePacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(updatePacket));

	*sendBuffer << updatePacket.size << updatePacket.type << timeStamp << entityId << pos << rot;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildDieNotifyPacket(ULONGLONG entityId)
{
	jh_network::DieNotifyPacket dieNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(dieNotifyPacket));

	*sendBuffer << dieNotifyPacket.size << dieNotifyPacket.type << entityId;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildSpectatorInitPacket()
{
	jh_network::SpectatorInitPacket spectatorInitPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(spectatorInitPacket));

	*sendBuffer << spectatorInitPacket.size << spectatorInitPacket.type;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildEnterGameResponsePacket()
{
	jh_network::EnterGameResponsePacket enterGameResponsePacket;

	PacketBufferRef buffer = jh_memory::MakeShared<PacketBuffer>(sizeof(enterGameResponsePacket));

	*buffer << enterGameResponsePacket.size << enterGameResponsePacket.type;

	return buffer;
}


PacketBufferRef jh_content::PacketBuilder::BuildGameEndNotifyPacket()
{
	jh_network::GameEndNotifyPacket gameEndNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(gameEndNotifyPacket));

	*sendBuffer << gameEndNotifyPacket.size << gameEndNotifyPacket.type;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildUpdateWinnerNotifyPacket(ULONGLONG userId, ULONGLONG expectedTimeStamp)
{
	jh_network::UpdateWinnerNotifyPacket updateWinnerNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(updateWinnerNotifyPacket));

	*sendBuffer << updateWinnerNotifyPacket.size << updateWinnerNotifyPacket.type << userId << expectedTimeStamp;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildInvalidateWinnerNotifyPacket(ULONGLONG canceledUserID)
{
	jh_network::InvalidateWinnerNotifyPacket invalidateWinnerNotifyPacket;
	invalidateWinnerNotifyPacket.canceledUserId = canceledUserID;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(invalidateWinnerNotifyPacket));

	*sendBuffer << invalidateWinnerNotifyPacket.size << invalidateWinnerNotifyPacket.type << invalidateWinnerNotifyPacket.canceledUserId;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildWinnerInfoNotifyPacket()
{
	jh_network::WinnerInfoNotifyPacket winnerInfoNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(winnerInfoNotifyPacket));

	*sendBuffer << winnerInfoNotifyPacket.size << winnerInfoNotifyPacket.type;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildCharacterSyncPacket(ULONGLONG entityId, const Vector3& syncPos, const Vector3& syncRot)
{
	jh_network::CharacterPositionSyncPacket syncPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(syncPacket));

	*sendBuffer << syncPacket.size << syncPacket.type << entityId << syncPos << syncRot;
	
	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildGameStartNotifyPacket()
{
	jh_network::GameStartNotifyPacket gameStartNotifyPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(gameStartNotifyPacket));

	*sendBuffer << gameStartNotifyPacket.size << gameStartNotifyPacket.type;
	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildGameServerSettingRequestPacket()
{
	jh_network::GameServerSettingRequestPacket settingRequestPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(settingRequestPacket));

	*sendBuffer << settingRequestPacket.size << settingRequestPacket.type;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildGameServerLanInfoPacket(const WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG token)
{
	jh_network::GameServerLanInfoPacket gameServerLanInfoPacket;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(gameServerLanInfoPacket));

	wcscpy_s(gameServerLanInfoPacket.ipStr, IP_STRING_LEN, ipStr);

	*sendBuffer << gameServerLanInfoPacket.size << gameServerLanInfoPacket.type;

	sendBuffer->PutData(reinterpret_cast<const char*>(gameServerLanInfoPacket.ipStr), IP_STRING_LEN * MESSAGE_SIZE);

	*sendBuffer << port << roomNum << token;

	return sendBuffer;
}

PacketBufferRef jh_content::PacketBuilder::BuildChatNotifyPacket(ULONGLONG userId, USHORT messageLen, const char* message)
{
	jh_network::ChatOtherUserNotifyPacket chatNotifyPacket;

	chatNotifyPacket.size = sizeof(userId) + sizeof(messageLen) + messageLen;
	chatNotifyPacket.type = jh_network::CHAT_NOTIFY_PACKET;

	PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(jh_network::PacketHeader) + chatNotifyPacket.size);
	*sendBuffer << chatNotifyPacket.size << chatNotifyPacket.type << userId << messageLen;

	sendBuffer->PutData(message, messageLen);

	return sendBuffer;
}
