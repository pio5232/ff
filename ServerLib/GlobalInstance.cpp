#include "LibraryPch.h"
#include "GlobalInstance.h"
#include "MemorySystem.h"

jh_utility::FileLogger* g_logger = nullptr;
jh_memory::MemorySystem* g_memSystem = nullptr;

//jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData>* g_packetPool = nullptr;


jh_utility::CrashDump dump;

class GlobalGenerator
{
public:
	GlobalGenerator()
	{
		PRO_RESET;

		g_memSystem = new jh_memory::MemorySystem();
		g_logger = new jh_utility::FileLogger();
		//g_packetPool = new jh_utility::ClearPool<jh_utility::SerializationBuffer>(true);

		//g_packetPool = new jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData>(0, &jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData::Clear);
		//g_jobPool = new jh_utility::NodeMemoryPool<jh_utility::Job>(0, true);
		//g_systemJobPool = new jh_utility::NodeMemoryPool<jh_utility::SessionConnectionEvent>(0, true);

		jh_network::NetAddress::Init();
		//PacketPtr::SetPool(g_packetPool);

	}

	~GlobalGenerator()
	{
		PRO_SAVE("ProfileData.TXT");

		delete g_logger;
		delete g_memSystem;
		//delete g_packetPool;
		//delete g_jobPool;
		//delete g_systemJobPool;
		jh_network::NetAddress::Clear();
	}
} g_gGlobalGen;