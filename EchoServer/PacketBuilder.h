#pragma once

namespace jh_content
{
	class PacketBuilder
	{
	public:
		//static jh_network::SerializationBufferPtr BuildEchoPacket(DWORD len, ULONGLONG data);
		static PacketBuffer* BuildEchoPacket(WORD len, ULONGLONG data);
	};
}
