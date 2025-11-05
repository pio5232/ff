#include "pch.h"
#include "PacketBuilder.h"
#include "Memory.h"
PacketPtr jh_content::PacketBuilder::BuildEchoPacket(WORD len, ULONGLONG data)
{
	//jh_utility::SerializationBuffer* pPacket = g_packetPool->Alloc();
	//PacketPtr packet = PacketPtr::Make();
	PacketPtr packet = MakeSharedBuffer(g_memSystem, 10);

	*packet << len << data;
	//*pPacket << len << data;

	_LOG(L"PacketBuilder", LOG_LEVEL_DETAIL, L"BuildEchoPacket - data : %u", data);

	//return pPacket;
	return packet;

}
