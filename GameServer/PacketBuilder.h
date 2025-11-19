#pragma once

namespace jh_content
{
	class PacketBuilder
	{
	public:
		static PacketRef BuildErrorPacket(jh_network::PacketErrorCode errorCode);

		static PacketRef BuildAttackNotifyPacket(ULONGLONG entityId);
		static PacketRef BuildAttackedNotifyPacket(ULONGLONG entityId, USHORT currentHp);

		static PacketRef BuildMakeMyCharacterPacket(ULONGLONG entityId, const Vector3& position);
		static PacketRef BuildMakeOtherCharacterPacket(ULONGLONG entityId, const Vector3& position);
		static PacketRef BuildGameInitDonePacket();
		static PacketRef BuildDeleteOtherCharacterPacket(ULONGLONG entityId);

		static PacketRef BuildMoveStartNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY);
		static PacketRef BuildMoveStopNotifyPacket(ULONGLONG entityId, const Vector3& pos, float rotY);
		static PacketRef BuildUpdateTransformPacket(ULONGLONG timeStamp, ULONGLONG entityId, const Vector3& pos, const Vector3& rot);
		static PacketRef BuildDieNotifyPacket(ULONGLONG entityId);
		static PacketRef BuildSpectatorInitPacket();

		static PacketRef BuildEnterGameResponsePacket();
		static PacketRef BuildGameEndNotifyPacket();
		static PacketRef BuildUpdateWinnerNotifyPacket(ULONGLONG userId, ULONGLONG expectedTimeStamp);
		static PacketRef BuildInvalidateWinnerNotifyPacket(ULONGLONG canceledUserId);
		static PacketRef BuildWinnerInfoNotifyPacket();

		static PacketRef BuildCharacterSyncPacket(ULONGLONG entityId, const Vector3& syncPos, const Vector3& syncRot);
		static PacketRef BuildGameStartNotifyPacket();

		static PacketRef BuildGameServerSettingRequestPacket();
		static PacketRef BuildGameServerLanInfoPacket(const WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG token);

		static PacketRef BuildChatNotifyPacket(ULONGLONG userId, USHORT messageLen, const char* message);
	};
}

