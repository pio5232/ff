#pragma once

namespace jh_content
{
	class PacketBuilder
	{
	public:
		static PacketBufferRef BuildEchoPacket(WORD len, ULONGLONG data);
	};
}
