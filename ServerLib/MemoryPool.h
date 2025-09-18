#pragma once
#include <cassert>

namespace jh_utility
{
	using Guard = unsigned long long;

	constexpr Guard guardValue = 0xffff'ffff'ffff'ffff;
	// ���� : MemoryPool�� �迭 ����� �ƴ� Node ������� ������.
	// 1. memorypool == objectpool�� ��� (����ȭ ����, ��ü���� Ǯ ����)
	// 
	template <typename T>
	class NodeMemoryPool
	{
		// malloc���� ���� ��� ����
		struct AllocatedMemoryPtr
		{
			void* memoryPtr;
			AllocatedMemoryPtr* m_pNext;
		};

		struct alignas(64) PoolNode
		{
#ifdef _DEBUG
			static constexpr DWORD GetFrontSize() { return sizeof(m_frontGuard); }
			Guard m_frontGuard;
			T m_data;
			Guard m_backGuard;
			PoolNode* m_pNext;
#else
			T data;
			PoolNode* m_pNext;
#endif 
		};

	public:

		NodeMemoryPool(DWORD BlockSize, bool bPlacementNew = false) : m_pNodeHead(nullptr), m_pAllocatedMemory(nullptr)
		{
			if (BlockSize < 0)
			{
				printf("[ NodeMemoryPool() Block Size < 0 !!\n");
				DebugBreak();
				return;
			}

			InitializeSRWLock(&m_lock);

			m_bPlacementNew = bPlacementNew;
			CreateNodes(BlockSize);
		}

		~NodeMemoryPool()
		{
			while (nullptr != m_pAllocatedMemory)
			{
				AllocatedMemoryPtr* current = m_pAllocatedMemory;

				m_pAllocatedMemory = m_pAllocatedMemory->m_pNext;

				_aligned_free(current->memoryPtr);
				//free (current->memoryPtr);
				free(current);
			}

			return;
		}

		template <typename ...Params>
		T* Alloc(Params&&... param)
		{
			T* ret;
			{
				SRWLockGuard lockGuard(&m_lock);
				
				if (nullptr == m_pNodeHead)
				{
					CreateNodes();
				}

				ret = reinterpret_cast<T*>(&m_pNodeHead->m_data); // ->m_data;

				m_pNodeHead = m_pNodeHead->m_pNext;

				m_lAllocCount++;
			}
			//placement new �ɼ�
			if (true == m_bPlacementNew)
				new (ret) T(std::forward<Params>(param)...);

			return ret;

		}

		void Free(T* ptr)
		{
			PoolNode* baseNodePtr;
#ifdef _DEBUG
			baseNodePtr = reinterpret_cast<PoolNode*>(reinterpret_cast<char*>(ptr) - PoolNode::GetFrontSize());

			if (baseNodePtr->m_frontGuard != guardValue)
			{
				// printf("Free -> Guard Corruption!! [FrontGuard] %0x\n", baseNodePtr->m_frontGuard);
				DebugBreak();
			}

			if (baseNodePtr->m_backGuard != guardValue)
			{
				// printf("Free -> Guard Corruption!! [BackGuard] %0x\n", baseNodePtr->m_backGuard);
				DebugBreak();
			}
#else
			baseNodePtr = reinterpret_cast<PoolNode*>(ptr);
#endif // _DEBUG
			//// placement new option
			if (true == m_bPlacementNew)
				ptr->~T();

			//PushNode(basePtr);

			SRWLockGuard lockGuard(&m_lock);

			baseNodePtr->m_pNext = m_pNodeHead;
			m_pNodeHead = baseNodePtr;

			m_lAllocCount--;
		}

		ULONG GetCreationCount() const { return m_lCreationCount; }
		ULONG GetAllocCount() const { return m_lAllocCount; }

	private:

		void CreateNodes(DWORD count = 1)
		{
			if (count == 0)
				return;

			// ù ����
			PoolNode* memoryPtr = static_cast<PoolNode*>(_aligned_malloc(sizeof(PoolNode) * count, 64));
			//PoolNode* memoryPtr = static_cast<PoolNode*>(malloc(sizeof(PoolNode) * count));
			if (nullptr == memoryPtr)
			{
				printf("CreateNodes - memoryPtr is Nullptr\n");
				return;
			}

#ifdef _DEBUG
			ZeroMemory(memoryPtr, sizeof(PoolNode) * count);
#endif // _DEBUG

			AllocatedMemoryPtr* allocatedMemoryPtr = reinterpret_cast<AllocatedMemoryPtr*>(malloc(sizeof(AllocatedMemoryPtr)));
			if (nullptr == allocatedMemoryPtr)
			{
				printf("CreateNodes - allocatedMemoryPtr is Nullptr\n");
				return;
			}


			for (int i = 0; i < count; i++)
			{
				// new �����ϸ� std::bad_alloc() �ڵ����� throw, �׳� �������� �д�.
#ifdef _DEBUG
				(memoryPtr + i)->m_frontGuard = guardValue;
				(memoryPtr + i)->m_backGuard = guardValue;
#endif // _DEBUG
				(memoryPtr + i)->m_pNext = m_pNodeHead;
				m_pNodeHead = (memoryPtr + i);
			}


			// ������ ����
			allocatedMemoryPtr->memoryPtr = memoryPtr;

			// Head ���� 
			allocatedMemoryPtr->m_pNext = m_pAllocatedMemory;
			m_pAllocatedMemory = allocatedMemoryPtr;

			m_lCreationCount += count;
		}

		PoolNode* m_pNodeHead;
		AllocatedMemoryPtr* m_pAllocatedMemory; // malloc���� ���� ��� ����

		bool m_bPlacementNew;
		ULONG m_lCreationCount;
		ULONG m_lAllocCount;

		alignas(64) SRWLOCK m_lock;

	};

	using DWORD = unsigned int;


	//// --------------------------------------------
	//// �迭 ������ �޸�Ǯ
	//// --------------------------------------------
	template <typename T, DWORD BlockSize = 1>
	class ArrayMemoryPool
	{
	public:
		ArrayMemoryPool() : m_dwIndexTableOffset(0)
		{
			m_pBaseMemory = static_cast<T*>(malloc(sizeof(T) * BlockSize));
			m_pIndexTable = static_cast<DWORD*>(malloc(sizeof(DWORD) * BlockSize));

			assert(m_pBaseMemory != nullptr);
			assert(m_pIndexTable != nullptr);

			for (int i = 0; i < BlockSize; i++)
			{
				m_pIndexTable[i] = i;
			}
		}

		~ArrayMemoryPool()
		{
			free(m_pBaseMemory);

			free(m_pIndexTable);
		}

		T* Alloc()
		{
			if (m_dwIndexTableOffset < 0 || m_dwIndexTableOffset >= BlockSize)
				return nullptr;

			T* availableMemory = m_pBaseMemory + m_pIndexTable[m_dwIndexTableOffset++];

			return availableMemory;
		}

		void Free(T* ptr)
		{
			DWORD tableIndex = ptr - m_pBaseMemory;

			// ������ ���̺� ����� �� �ִ� �ּҸ� ����Ѵ�. ( T* ���� + N��° ��� �ǹ�)
			m_pIndexTable[--m_dwIndexTableOffset] = tableIndex;

			return;
		}
	private:
		DWORD* m_pIndexTable;

		DWORD m_dwIndexTableOffset; // indexTable ������ ���� �ε���

		T* m_pBaseMemory;
	};
}