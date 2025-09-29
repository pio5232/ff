#pragma once
namespace jh_utility
{
	class SerializationBuffer;
	class Job;
	class SessionConnectionEvent;
	class FileLogger;
}

extern jh_memory::MemoryAllocator* g_memAllocator;
extern jh_utility::FileLogger* g_logger;
//extern jh_utility::NodeMemoryPool<jh_utility::SerializationBuffer>* g_packetPool;

// TODO : 에러터진다.. 수정하자
//extern jh_utility::TLSArrayMemoryPool <jh_utility::SerializationBuffer>* g_packetPool;

//extern jh_utility::ClearPool <jh_utility::SerializationBuffer>* g_packetPool;
//extern jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData>* g_packetPool;

// ------
//extern jh_utility::NodeMemoryPool<jh_utility::Job>* g_jobPool;
//extern jh_utility::NodeMemoryPool<jh_utility::SessionConnectionEvent>* g_systemJobPool;

//extern jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::Job>::RefData>* g_jobPool;
//extern jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::SessionConnectionEvent>::RefData>* g_systemJobPool;
