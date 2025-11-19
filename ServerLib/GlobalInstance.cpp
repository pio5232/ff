#include "LibraryPch.h"
#include "GlobalInstance.h"
#include "MemorySystem.h"

jh_utility::FileLogger* g_logger		= nullptr;
jh_memory::MemorySystem* g_memSystem	= nullptr;

jh_utility::CrashDump dump;

class GlobalGenerator
{
public:
	GlobalGenerator()
	{
		PRO_RESET;

		g_memSystem = new jh_memory::MemorySystem();
		g_logger = new jh_utility::FileLogger();

		jh_network::NetAddress::Init();
	}

	~GlobalGenerator()
	{
		PRO_SAVE("ProfileData.TXT");

		delete g_logger;
		delete g_memSystem;
		
		jh_network::NetAddress::Clear();
	}
} g_gGlobalGen;