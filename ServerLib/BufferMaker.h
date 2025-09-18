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


		// 직렬화 버퍼에 데이터를 채우자! 가변 템플릿을 활용. 길이가 고정되어 있는 경우는 값을 넣어놓고 이 함수를 통해 send.
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
