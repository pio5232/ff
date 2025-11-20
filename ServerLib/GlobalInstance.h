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

extern jh_memory::MemorySystem	* g_memSystem;
extern jh_utility::FileLogger	* g_logger;
