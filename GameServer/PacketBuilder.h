#pragma once

namespace jh_content
{
	class PacketBuilder
	{
	public:
		static PacketBufferRef BuildErrorPacket(jh_network::PacketErrorCode errorCode);

		static PacketBufferRef BuildAttackNotifyPacket(ULONGLONG entityId);
		static PacketBufferRef BuildAttackedNotifyPacket(ULONGLONG entityId, USHORT currentHp);

		static PacketBufferRef BuildMakeMyCharacterPacket(ULONGLONG entityId, const Vector3& position);
		static PacketBufferRef BuildMakeOtherCharacterPacket(ULONGLONG entityId, const Vector3& position);
		static PacketBufferRef BuildGameInitDonePacket();
		static PacketBufferRef BuildDeleteOtherCharacterPacket(ULONGLONG entityId);

		static PacketBufferRef BuildMoveStartNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY);
		static PacketBufferRef BuildMoveStopNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY);
		static PacketBufferRef BuildUpdateTransformPacket(ULONGLONG timeStamp, ULONGLONG entityId, const Vector3& pos, const Vector3& rot);
		static PacketBufferRef BuildDieNotifyPacket(ULONGLONG entityId);
		static PacketBufferRef BuildSpectatorInitPacket();

		static PacketBufferRef BuildEnterGameResponsePacket();
		static PacketBufferRef BuildGameEndNotifyPacket();
		static PacketBufferRef BuildUpdateWinnerNotifyPacket(ULONGLONG userId, ULONGLONG expectedTimeStamp);
		static PacketBufferRef BuildInvalidateWinnerNotifyPacket(ULONGLONG canceledUserId);
		static PacketBufferRef BuildWinnerInfoNotifyPacket();

		static PacketBufferRef BuildCharacterSyncPacket(ULONGLONG entityId, const Vector3& syncPos, const Vector3& syncRot);
		static PacketBufferRef BuildGameStartNotifyPacket();

		static PacketBufferRef BuildGameServerSettingRequestPacket();
		static PacketBufferRef BuildGameServerLanInfoPacket(const WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG token);

		static PacketBufferRef BuildChatNotifyPacket(ULONGLONG userId, USHORT messageLen, const char* message);
	};
}

