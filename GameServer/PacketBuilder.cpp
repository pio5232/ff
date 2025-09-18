#include "pch.h"
#include "PacketBuilder.h"
#include "BufferMaker.h"
#include "Memory.h"
PacketPtr jh_content::PacketBuilder::BuildAttackNotifyPacket(ULONGLONG entityId)
{
	jh_network::AttackNotifyPacket attackNotifyPacket;
	attackNotifyPacket.entityId = entityId;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(attackNotifyPacket));

	*sendBuffer << attackNotifyPacket.size << attackNotifyPacket.type << attackNotifyPacket.entityId;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildAttackedNotifyPacket(ULONGLONG entityId, USHORT currentHp)
{
	jh_network::AttackedNotifyPacket attackedNotifyPacket;
	attackedNotifyPacket.entityId = entityId;
	attackedNotifyPacket.currentHp = currentHp;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(attackedNotifyPacket));

	*sendBuffer << attackedNotifyPacket.size << attackedNotifyPacket.type << attackedNotifyPacket.entityId << attackedNotifyPacket.currentHp;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMakeMyCharacterPacket(ULONGLONG entityId, const Vector3& position)
{
	jh_network::MakeMyCharacterPacket makeMyCharacterPacket;

	makeMyCharacterPacket.entityId = entityId;
	makeMyCharacterPacket.pos = position;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(makeMyCharacterPacket));

	*sendBuffer << makeMyCharacterPacket.size << makeMyCharacterPacket.type << makeMyCharacterPacket.entityId << makeMyCharacterPacket.pos;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(ULONGLONG entityId, const Vector3& position)
{
	jh_network::MakeOtherCharacterPacket makeOtherCharacterPacket;

	makeOtherCharacterPacket.entityId = entityId;
	makeOtherCharacterPacket.pos = position;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(makeOtherCharacterPacket));

	*sendBuffer << makeOtherCharacterPacket.size << makeOtherCharacterPacket.type << makeOtherCharacterPacket.entityId << makeOtherCharacterPacket.pos;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameInitDonePacket()
{
	jh_network::GameInitDonePacket donePacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(donePacket));

	*sendBuffer << donePacket.size << donePacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(ULONGLONG entityId)
{
	jh_network::DeleteOtherCharacterPacket deleteOtherCharacterPacket;
	deleteOtherCharacterPacket.entityId = entityId;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(deleteOtherCharacterPacket));

	*sendBuffer << deleteOtherCharacterPacket.size << deleteOtherCharacterPacket.type << deleteOtherCharacterPacket.entityId;
	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMoveStartNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY)
{
	jh_network::MoveStartNotifyPacket moveStartNotifyPacket;
	moveStartNotifyPacket.entityId = entityId;
	moveStartNotifyPacket.pos = pos;
	moveStartNotifyPacket.rotY = rotY;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(moveStartNotifyPacket));

	*sendBuffer << moveStartNotifyPacket.size << moveStartNotifyPacket.type << moveStartNotifyPacket.entityId << moveStartNotifyPacket.pos << moveStartNotifyPacket.rotY;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildMoveStopNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY)
{
	jh_network::MoveStopNotifyPacket moveStopNotifyPacket;
	moveStopNotifyPacket.entityId = entityId;
	moveStopNotifyPacket.pos = pos;
	moveStopNotifyPacket.rotY = rotY;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(moveStopNotifyPacket));

	*sendBuffer << moveStopNotifyPacket.size << moveStopNotifyPacket.type << moveStopNotifyPacket.entityId
		<< moveStopNotifyPacket.pos << moveStopNotifyPacket.rotY;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildUpdateTransformPacket(ULONGLONG timeStamp, ULONGLONG entityId, const Vector3& pos, const Vector3& rot)
{
	jh_network::UpdateTransformPacket updatePacket;

	updatePacket.timeStamp = timeStamp;
	updatePacket.entityId = entityId;
	updatePacket.pos = pos;
	updatePacket.rot = rot;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(updatePacket));

	*sendBuffer << updatePacket.size << updatePacket.type << updatePacket.timeStamp << updatePacket.entityId << updatePacket.pos
		<< updatePacket.rot;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildDieNotifyPacket(ULONGLONG entityId)
{
	jh_network::DieNotifyPacket dieNotifyPacket;
	dieNotifyPacket.entityId = entityId;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(dieNotifyPacket));

	*sendBuffer << dieNotifyPacket.size << dieNotifyPacket.type << dieNotifyPacket.entityId;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildSpectatorInitPacket()
{
	jh_network::SpectatorInitPacket spectatorInitPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(spectatorInitPacket));

	*sendBuffer << spectatorInitPacket.size << spectatorInitPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildEnterGameResponsePacket()
{
	jh_network::EnterGameResponsePacket enterGameResponsePacket;

	PacketPtr buffer = MakeSharedBuffer(g_memAllocator, sizeof(enterGameResponsePacket));

	*buffer << enterGameResponsePacket.size << enterGameResponsePacket.type;

	return buffer;
}


PacketPtr jh_content::PacketBuilder::BuildGameEndNotifyPacket()
{
	jh_network::GameEndNotifyPacket gameEndNotifyPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(gameEndNotifyPacket));

	*sendBuffer << gameEndNotifyPacket.size << gameEndNotifyPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildUpdateWinnerNotifyPacket(ULONGLONG userId, ULONGLONG expectedTimeStamp)
{
	jh_network::UpdateWinnerNotifyPacket updateWinnerNotifyPacket;

	updateWinnerNotifyPacket.userId = userId;
	updateWinnerNotifyPacket.expectedTimeStamp = expectedTimeStamp;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(updateWinnerNotifyPacket));

	*sendBuffer << updateWinnerNotifyPacket.size << updateWinnerNotifyPacket.type << updateWinnerNotifyPacket.userId << updateWinnerNotifyPacket.expectedTimeStamp;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildInvalidateWinnerNotifyPacket(ULONGLONG canceledUserID)
{
	jh_network::InvalidateWinnerNotifyPacket invalidateWinnerNotifyPacket;
	invalidateWinnerNotifyPacket.canceledUserId = canceledUserID;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(invalidateWinnerNotifyPacket));

	*sendBuffer << invalidateWinnerNotifyPacket.size << invalidateWinnerNotifyPacket.type << invalidateWinnerNotifyPacket.canceledUserId;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildWinnerInfoNotifyPacket()
{
	jh_network::WinnerInfoNotifyPacket winnerInfoNotifyPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(winnerInfoNotifyPacket));

	*sendBuffer << winnerInfoNotifyPacket.size << winnerInfoNotifyPacket.type;

	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameStartNotifyPacket()
{
	jh_network::GameStartNotifyPacket gameStartNotifyPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(gameStartNotifyPacket));

	*sendBuffer << gameStartNotifyPacket.size << gameStartNotifyPacket.type;
	return sendBuffer;
}

PacketPtr jh_content::PacketBuilder::BuildGameServerLanInfoPacket(const WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG token)
{
	jh_network::GameServerLanInfoPacket gameServerLanInfoPacket;

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(gameServerLanInfoPacket));

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

	PacketPtr sendBuffer = MakeSharedBuffer(g_memAllocator, sizeof(jh_network::PacketHeader) + chatNotifyPacket.size);
	*sendBuffer << chatNotifyPacket.size << chatNotifyPacket.type << userId << messageLen;

	sendBuffer->PutData(message, messageLen);

	return sendBuffer;
}
