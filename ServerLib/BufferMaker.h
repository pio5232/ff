#pragma once
#include "PacketDefine.h"

namespace jh_network
{
	class BufferMaker
	{
	public:
		//static SerializationBufferPtr MakeSendBuffer(DWORD packetSize)
		//{
		//	return std::make_shared<jh_utility::SerializationBuffer>(packetSize);
		//}


		// ����ȭ ���ۿ� �����͸� ä����! ���� ���ø��� Ȱ��. ���̰� �����Ǿ� �ִ� ���� ���� �־���� �� �Լ��� ���� send.
		//template <typename PacketType>
		//static SerializationBufferPtr MakePacket(PacketType& packet)// (WORD packetType, PacketType& packet)
		//{
		//	// AA
		//	WORD packetSize = sizeof(packet);

		//	SerializationBufferPtr sendBuffer = std::make_shared<jh_utility::SerializationBuffer>(packetSize);

		//	*sendBuffer << packet;

		//	return sendBuffer;
		//}

	};
}
