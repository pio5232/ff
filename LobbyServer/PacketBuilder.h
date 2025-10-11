#pragma once
namespace jh_content
{
	class PacketBuilder
	{
	public:
		static PacketPtr BuildErrorPacket(jh_network::PacketErrorCode errorCode);

		static PacketPtr BuildLogInResponsePacket(ULONGLONG userId);
		static PacketPtr BuildMakeRoomResponsePacket(bool isMade, ULONGLONG ownerId, USHORT roomNum, USHORT curUserCnt, USHORT maxUserCnt, const WCHAR* roomName);
		static PacketPtr BuildEnterRoomNotifyPacket(ULONGLONG userId);
		static PacketPtr BuildEnterRoomResponsePacket(bool isAllowed, const std::vector<ULONGLONG>& userIdAndReadyList); // 입장 요청한 유저에게 입장 가능 여부, 유저 목록을 전송.
		static PacketPtr BuildLeaveRoomNotifyPacket(ULONGLONG userId);
		static PacketPtr BuildLeaveRoomResponsePacket();
		static PacketPtr BuildRoomListResponsePacket(std::vector<jh_content::RoomInfo>& roomInfos);
		static PacketPtr BuildOwnerChangeNotifyPacket(ULONGLONG newOwnerUserId);
		
		static PacketPtr BuildGameStartNotifyPacket();
		static PacketPtr BuildGameReadyNotifyPacket(ULONGLONG userId, bool isReady);

		static PacketPtr BuildLanInfoPacket(WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG tok);
		static PacketPtr BuildGameServerSettingResponsePacket(USHORT roomNum, USHORT requiredUserCnt, USHORT maxUserCnt);

	};
}


