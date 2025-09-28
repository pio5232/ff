#pragma once

namespace jh_content
{
	class PacketBuilder
	{
	public:
		static PacketPtr BuildErrorPacket(jh_network::PacketErrorCode errorCode);

		static PacketPtr BuildAttackNotifyPacket(ULONGLONG entityId);
		static PacketPtr BuildAttackedNotifyPacket(ULONGLONG entityId, USHORT currentHp);

		static PacketPtr BuildMakeMyCharacterPacket(ULONGLONG entityId, const Vector3& position);
		static PacketPtr BuildMakeOtherCharacterPacket(ULONGLONG entityId, const Vector3& position);
		static PacketPtr BuildGameInitDonePacket();
		static PacketPtr BuildDeleteOtherCharacterPacket(ULONGLONG entityId);

		static PacketPtr BuildMoveStartNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY);
		static PacketPtr BuildMoveStopNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY);
		static PacketPtr BuildUpdateTransformPacket(ULONGLONG timeStamp, ULONGLONG entityId, const Vector3& pos, const Vector3& rot);
		static PacketPtr BuildDieNotifyPacket(ULONGLONG entityId);
		static PacketPtr BuildSpectatorInitPacket();

		static PacketPtr BuildEnterGameResponsePacket();
		static PacketPtr BuildGameEndNotifyPacket();
		static PacketPtr BuildUpdateWinnerNotifyPacket(ULONGLONG userId, ULONGLONG expectedTimeStamp);
		static PacketPtr BuildInvalidateWinnerNotifyPacket(ULONGLONG canceledUserId);
		static PacketPtr BuildWinnerInfoNotifyPacket();

		static PacketPtr BuildCharacterSyncPacket(ULONGLONG entityId, const Vector3& syncPos, const Vector3& syncRot);
		static PacketPtr BuildGameStartNotifyPacket();

		static PacketPtr BuildGameServerSettingRequestPacket();
		static PacketPtr BuildGameServerLanInfoPacket(const WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG token);

		static PacketPtr BuildChatNotifyPacket(ULONGLONG userId, USHORT messageLen, const char* message);
	};
}

