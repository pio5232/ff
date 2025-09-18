#pragma once

namespace jh_content
{
	class PacketBuilder
	{
	public:
		//static jh_network::SerializationBufferPtr BuildEchoPacket(DWORD len, ULONGLONG data);
		static PacketPtr BuildEchoPacket(WORD len, ULONGLONG data);
	};
}
