#include "LibraryPch.h"
#include "GlobalInstance.h"
#include "Memory.h"

jh_utility::FileLogger* g_logger = nullptr;
jh_memory::MemoryAllocator* g_memAllocator = nullptr;

//jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData>* g_packetPool = nullptr;

jh_utility::NodeMemoryPool<jh_utility::Job>* g_jobPool;
jh_utility::NodeMemoryPool<jh_utility::SessionConnectionEvent>* g_systemJobPool;

class CGlobalGen
{
public:
	CGlobalGen()
	{

		g_logger = new jh_utility::FileLogger();
		//g_packetPool = new jh_utility::ClearPool<jh_utility::SerializationBuffer>(true);
		g_memAllocator = new jh_memory::MemoryAllocator();

		//g_packetPool = new jh_utility::ClearPool<jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData>(0, &jh_utility::ClearRefPtr<jh_utility::SerializationBuffer>::RefData::Clear);
		g_jobPool = new jh_utility::NodeMemoryPool<jh_utility::Job>(0, true);
		g_systemJobPool = new jh_utility::NodeMemoryPool<jh_utility::SessionConnectionEvent>(0, true);

		jh_network::NetAddress::Init();
		//PacketPtr::SetPool(g_packetPool);

	}

	~CGlobalGen()
	{
		delete g_memAllocator;
		//delete g_packetPool;
		delete g_jobPool;
		delete g_systemJobPool;

		jh_network::NetAddress::Clear();
	}
} g_GlobalGen;