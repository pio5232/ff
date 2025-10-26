#pragma once

using MemoryGuard = unsigned int;
//using WORD = unsigned short;

using uint64 = unsigned long long;
enum : unsigned
{
	MESSAGE_SIZE = sizeof(WCHAR),
	IP_STRING_LEN = 22,
	USER_NAME_MAX_LEN = 20,
	ROOM_NAME_MAX_LEN = 20 + 1,
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
	//using SerializationBufferPtr = std::shared_ptr<class jh_utility::SerializationBuffer>;
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
	
	//class ClearRefPtr<SerializationBuffer>;
}
const ULONGLONG xorTokenKey = 0x0123456789ABCDEF;

//using SessionPtr = std::shared_ptr<class jh_network::Session>;

using PacketPtr = std::shared_ptr<class jh_utility::SerializationBuffer>;
// using PacketPtr = jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>;
using JobPtr = std::shared_ptr<class jh_utility::Job>;
using SessionConnectionEventPtr = std::shared_ptr<class jh_utility::SessionConnectionEvent>;


//void FreeJob(jh_utility::Job* pJob);
//
//void FreeSystemJob(jh_utility::SessionConnectionEvent* pSystemJob);;
//
//template <typename... Params>
//JobPtr MakeJob(Params&&... param)
//{
//	return JobPtr(g_jobPool->Alloc(std::forward<Params>(param)...),FreeJob);
//}
//
//template <typename... Params>
//SessionConnectionEventPtr MakeSystemJob(Params&&... param)
//{
//	return SessionConnectionEventPtr(g_systemJobPool->Alloc(std::forward<Params>(param)...), FreeSystemJob);
//}


