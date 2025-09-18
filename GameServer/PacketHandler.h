#pragma once

namespace jh_network
{

	class GameClientPacketHandler
	{
	public:
		using PacketFunc = ErrorCode(*)(GameSessionPtr&, jh_utility::SerializationBuffer&);

		static void Init();

		static ErrorCode ProcessPacket(GameSessionPtr& gameSessionPtr, uint16 packetType, jh_utility::SerializationBuffer& buffer)
		{
			if (_packetFuncsDic.find(packetType) == _packetFuncsDic.end())
				return ErrorCode::CANNOT_FIND_PACKET_FUNC;

			return _packetFuncsDic[packetType](gameSessionPtr, buffer);

		}

		static ErrorCode ProcessEnterGameRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer);
		static ErrorCode ProcessLoadCompletedPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer);

		static ErrorCode ProcessMoveStartRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer);
		static ErrorCode ProcessMoveStopRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer);
		static ErrorCode ProcessChatRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer);

		static ErrorCode ProcessAttackRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer);
	private:
		static std::unordered_map<uint16, PacketFunc> _packetFuncsDic;

	};


	class LanServerPacketHandler
	{
	public:
		using PacketFunc = ErrorCode(*)(LanClientSessionPtr&, jh_utility::SerializationBuffer&);

		static void Init();

		static ErrorCode ProcessPacket(LanClientSessionPtr& lanClientSessionPtr, uint16 packetType, jh_utility::SerializationBuffer& buffer)
		{
			if (_packetFuncsDic.find(packetType) == _packetFuncsDic.end())
				return ErrorCode::CANNOT_FIND_PACKET_FUNC;

			return _packetFuncsDic[packetType](lanClientSessionPtr, buffer);

		}

		static ErrorCode ProcessGameServerSettingResponsePacket(LanClientSessionPtr& lanClientSessionPtr, jh_utility::SerializationBuffer& buffer);

	private:
		static std::unordered_map<uint16, PacketFunc> _packetFuncsDic;

	};

}