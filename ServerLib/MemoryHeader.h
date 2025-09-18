#pragma once

namespace jh_memory
{
	class MemoryPool;

	struct MemoryHeader
	{
		// [MemoryHeader][Data]
		MemoryHeader(MemoryPool* pOwnerPool) : m_pOwnerPool(pOwnerPool) {}

		static void* AttachHeader(MemoryHeader* header, MemoryPool* owner)
		{
			// placement new
			new(header)MemoryHeader(owner);

			return reinterpret_cast<void*>(++header);
		}

		static MemoryHeader* DetachHeader(void* ptr)
		{
			return reinterpret_cast<MemoryHeader*>(ptr) - 1;
		}

		MemoryPool* m_pOwnerPool;
	};
}