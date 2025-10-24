#pragma once
namespace jh_content
{
	class DummyPacketBuilder
	{
	public:
		//static jh_network::SerializationBufferPtr BuildEchoPacket(DWORD len, ULONGLONG data);
		static PacketPtr BuildLoginRequestPacket();
		static PacketPtr BuildHeartbeatPacket(ULONGLONG timeStamp);
		static PacketPtr BuildMakeRoomRequestPacket();
		static PacketPtr BuildEnterRoomRequestPacket(USHORT roomNum, const WCHAR* roomName);
		static PacketPtr BuildLeaveRoomRequestPacket(USHORT roomNum, const WCHAR* roomName);
		static PacketPtr BuildChatRequestPacket(USHORT roomNum);
		static PacketPtr BuildRoomListRequestPacket();

		static PacketPtr BuildEchoPacket();
	};
}