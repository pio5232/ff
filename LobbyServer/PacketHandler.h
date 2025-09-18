#pragma once
//
//#include <functional>
//#include <unordered_map>
//
//namespace jh_content
//{
//	class LanSystem
//	{
//	public:
//		using PacketFunc = ErrorCode(LanSystem::*)(ULONGLONG sessionId, jh_utility::SerializationBuffer);
//
//		static void Init();
//
//		ErrorCode ProcessPacket(ULONGLONG sessionId, USHORT packetType, jh_utility::SerializationBuffer buffer)
//		{
//			if (m_packetFuncsDic.find(packetType) == m_packetFuncsDic.end())
//				return ErrorCode::CANNOT_FIND_PACKET_FUNC;
//
//			return (this->*m_packetFuncsDic[packetType])(sessionId, buffer);
//
//		}
//
//		ErrorCode ProcessLanInfoNotifyPacket(ULONGLONG sessionId, jh_utility::SerializationBuffer buffer);
//		ErrorCode ProcessGameSettingRequestPacket(ULONGLONG sessionId, jh_utility::SerializationBuffer buffer);
//
//	private:
//		std::unordered_map<USHORT, PacketFunc> m_packetFuncsDic;
//
//	};
//
//
//
//}
