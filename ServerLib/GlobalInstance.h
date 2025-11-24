#pragma once
namespace jh_utility
{
	class SerializationBuffer;
	class Job;
	class SessionConnectionEvent;
	class FileLogger;
}

namespace jh_memory
{
	class MemorySystem;
}

namespace jh_content
{
	class GlobalQueue;
	class JobTimer;
}

extern jh_memory::MemorySystem				* g_pMemSystem;
extern jh_utility::FileLogger				* g_pLogger;

extern jh_content::GlobalQueue				* g_pGlobalQueue;
extern jh_content::JobTimer					* g_pJobTimer;
extern thread_local ULONGLONG				g_tlsEndTickCount;
extern thread_local jh_content::JobQueue	* g_tlsCurrentJobQueue;
