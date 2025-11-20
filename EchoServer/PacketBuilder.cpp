#include "pch.h"
#include "PacketBuilder.h"
#include "Memory.h"
PacketBufferRef jh_content::PacketBuilder::BuildEchoPacket(WORD len, ULONGLONG data)
{
	PacketBufferRef packet = jh_memory::MakeShared<PacketBuffer>(10);

	*packet << len << data;

	_LOG(L"PacketBuilder", LOG_LEVEL_DEBUG, L"[BuildEchoPacket] data: [%u]", data);

	return packet;

}
