#pragma once
#include <cassert>

namespace jh_utility
{
	using Guard = unsigned long long;

	constexpr Guard guardValue = 0xffff'ffff'ffff'ffff;
	// 공통 : MemoryPool은 배열 기반이 아닌 Node 기반으로 만들어본다.
	// 1. memorypool == objectpool인 경우 (단편화 없음, 객체마다 풀 생성)
	// 
	template <typename T>
	class NodeMemoryPool
	{
		// malloc으로 받은 덩어리 관리
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
			//placement new 옵션
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

			// 첫 연결
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
				// new 실패하면 std::bad_alloc() 자동으로 throw, 그냥 터지도록 둔다.
#ifdef _DEBUG
				(memoryPtr + i)->m_frontGuard = guardValue;
				(memoryPtr + i)->m_backGuard = guardValue;
#endif // _DEBUG
				(memoryPtr + i)->m_pNext = m_pNodeHead;
				m_pNodeHead = (memoryPtr + i);
			}


			// 데이터 삽입
			allocatedMemoryPtr->memoryPtr = memoryPtr;

			// Head 변경 
			allocatedMemoryPtr->m_pNext = m_pAllocatedMemory;
			m_pAllocatedMemory = allocatedMemoryPtr;

			m_lCreationCount += count;
		}

		PoolNode* m_pNodeHead;
		AllocatedMemoryPtr* m_pAllocatedMemory; // malloc으로 받은 덩어리 관리

		bool m_bPlacementNew;
		ULONG m_lCreationCount;
		ULONG m_lAllocCount;

		alignas(64) SRWLOCK m_lock;

	};

	using DWORD = unsigned int;


	//// --------------------------------------------
	//// 배열 버전의 메모리풀
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

			// 포인터 테이블에 사용할 수 있는 주소를 등록한다. ( T* 기준 + N번째 라는 의미)
			m_pIndexTable[--m_dwIndexTableOffset] = tableIndex;

			return;
		}
	private:
		DWORD* m_pIndexTable;

		DWORD m_dwIndexTableOffset; // indexTable 참조를 위한 인덱스

		T* m_pBaseMemory;
	};
}