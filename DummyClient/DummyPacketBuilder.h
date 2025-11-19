#pragma once
namespace jh_content
{
	class DummyPacketBuilder
	{
	public:
		//static jh_network::SerializationBufferPtr BuildEchoPacket(DWORD len, ULONGLONG data);
		static PacketRef BuildLoginRequestPacket();
		static PacketRef BuildHeartbeatPacket(ULONGLONG timeStamp);
		static PacketRef BuildMakeRoomRequestPacket();
		static PacketRef BuildEnterRoomRequestPacket(USHORT roomNum, const WCHAR* roomName);
		static PacketRef BuildLeaveRoomRequestPacket(USHORT roomNum, const WCHAR* roomName);
		static PacketRef BuildChatRequestPacket(USHORT roomNum);
		static PacketRef BuildRoomListRequestPacket();

		static PacketRef BuildEchoPacket();
	};
}