#include "pch.h"
#include "PacketBuilder.h"
#include "Memory.h"
PacketPtr jh_content::PacketBuilder::BuildEchoPacket(WORD len, ULONGLONG data)
{
	PacketPtr packet = MakeSharedBuffer(g_memSystem, 10);

	*packet << len << data;

	_LOG(L"PacketBuilder", LOG_LEVEL_DEBUG, L"[BuildEchoPacket] data: [%u]", data);

	return packet;

}
