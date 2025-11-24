#include "LibraryPch.h"
#include "GlobalInstance.h"
#include "GlobalQueue.h"
#include "JobTimer.h"

jh_utility::FileLogger* g_pLogger = nullptr;
jh_memory::MemorySystem* g_pMemSystem = nullptr;

extern thread_local ULONGLONG g_tlsEndTickCount = 0;
extern thread_local jh_content::JobQueue* g_tlsCurrentJobQueue = nullptr;



jh_utility::CrashDump dump;

class GlobalGenerator
{
public:
	GlobalGenerator()
	{
		PRO_RESET;

		g_pMemSystem = new jh_memory::MemorySystem();
		g_pLogger = new jh_utility::FileLogger();
		g_pGlobalQueue = new jh_content::GlobalQueue();
		g_pJobTimer = new jh_content::JobTimer();
		jh_network::NetAddress::Init();
	}

	~GlobalGenerator()
	{
		PRO_SAVE("ProfileData.TXT");

		delete g_pLogger;
		delete g_pMemSystem;

		jh_network::NetAddress::Clear();
	}
} g_globalGen;