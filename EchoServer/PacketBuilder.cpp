#include "pch.h"
#include "PacketBuilder.h"
#include "ObjectPool.h"
PacketBuffer* jh_content::PacketBuilder::BuildEchoPacket(WORD len, ULONGLONG data)
{
	PacketBuffer* packet = MakePacketBuffer(10);

	*packet << len << data;

	_LOG(L"PacketBuilder", LOG_LEVEL_DEBUG, L"[BuildEchoPacket] data: [%u]", data);

	return packet;

}
