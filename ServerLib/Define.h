#pragma once

using MemoryGuard	= unsigned int;
using uint64		= unsigned long long;

enum : unsigned
{
	MESSAGE_SIZE		= sizeof(WCHAR),
	IP_STRING_LEN		= 22,
	USER_NAME_MAX_LEN	= 20,
	ROOM_NAME_MAX_LEN	= 20 + 1,
};

namespace jh_utility
{
	class SerializationBuffer;
	class Job;
	class SessionConnectionEvent;
}

namespace jh_network
{
	class Session;
	using SharedSendBufChunk = std::shared_ptr<class SendBufferChunk>;
}

namespace jh_content
{
	class Room;
	class User;

	struct RoomInfo
	{
		static const USHORT GetSize() { return sizeof(m_ullOwnerId) + sizeof(m_usRoomNum) + sizeof(m_usCurUserCnt) + sizeof(m_usMaxUserCnt) + sizeof(m_wszRoomName); }
		ULONGLONG m_ullOwnerId{};
		USHORT m_usRoomNum{};
		USHORT m_usCurUserCnt{};
		USHORT m_usMaxUserCnt{};
		WCHAR m_wszRoomName[ROOM_NAME_MAX_LEN]{};
	};
}

namespace jh_utility
{
	class SerializationBuffer;
	
}
const ULONGLONG xorTokenKey = 0x0123456789ABCDEF;

using PacketBuffer				= class jh_utility::SerializationBuffer;

using PacketBufferRef			= std::shared_ptr<class jh_utility::SerializationBuffer>;
using JobRef					= std::shared_ptr<class jh_utility::Job>;
using SessionConnectionEventRef	= std::shared_ptr<class jh_utility::SessionConnectionEvent>;


