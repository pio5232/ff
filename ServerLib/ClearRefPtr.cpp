#include "LibraryPch.h"
#include "ClearRefPtr.h"


jh_utility::ClearPool<PacketPtr::RefData>* PacketPtr::m_pClearPool = nullptr;