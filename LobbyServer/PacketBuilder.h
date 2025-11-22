#pragma once
namespace jh_content
{
	class PacketBuilder
	{
	public:
		static PacketBufferRef BuildErrorPacket(jh_network::PacketErrorCode errorCode);

		static PacketBufferRef BuildLogInResponsePacket(ULONGLONG userId);
		static PacketBufferRef BuildMakeRoomResponsePacket(bool isMade, ULONGLONG ownerId, USHORT roomNum, USHORT curUserCnt, USHORT maxUserCnt, const WCHAR* roomName);
		static PacketBufferRef BuildEnterRoomNotifyPacket(ULONGLONG userId);
		static PacketBufferRef BuildEnterRoomResponsePacket(bool isAllowed, const std::vector<ULONGLONG>& userIdAndReadyList); // 입장 요청한 유저에게 입장 가능 여부, 유저 목록을 전송.
		static PacketBufferRef BuildLeaveRoomNotifyPacket(ULONGLONG userId);
		static PacketBufferRef BuildLeaveRoomResponsePacket();
		static PacketBufferRef BuildRoomListResponsePacket(std::vector<jh_content::RoomInfo>& roomInfos);
		static PacketBufferRef BuildOwnerChangeNotifyPacket(ULONGLONG newOwnerUserId);
		
		static PacketBufferRef BuildGameStartNotifyPacket();
		static PacketBufferRef BuildGameReadyNotifyPacket(ULONGLONG userId, bool isReady);
		
		static PacketBufferRef BuildEchoPacket(ULONGLONG data);

		static PacketBufferRef BuildLanInfoPacket(WCHAR* ipStr, USHORT port, USHORT roomNum, ULONGLONG tok);
		static PacketBufferRef BuildGameServerSettingResponsePacket(USHORT roomNum, USHORT requiredUserCnt, USHORT maxUserCnt);

	};
}


