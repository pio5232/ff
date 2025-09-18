#pragma once

#include "LFMemoryPool.h"

//#define MYLOG

namespace jh_utility
{
	// �迭 ������ TLS �޸�Ǯ. ALLOC�� FREE�� ���� �۾��� �и�.
	// �̰��� ����Ҷ� capacity��ŭ �Ҵ� -> �ٸ� Chunk�� �ٲ۴�.
	// �ٸ� Chunk�� �ٲ� ���¿��� ���� �����Ϳ� ���� ���� -> �� �����忡�� �����ص� interlocke���� Free Count�� �÷��ְ�
	// �������� ������ �����尡 �ٽ� ������Ǯ�� �ֵ��� �Ѵ�.

	// �׷��� ���� �������� �� �Ҵ�/������ ���ؼ� �� �����尡 �������� �ʰų� ������ �Ҵ� ��Ȳ������
	// +------------------------------------------------------------+
	// | ���� Chunk |    | ���ο� Chunk |
	// +------------+    +--------------+
	//                          ��
	//                   +--------------+
	//                   |   �� ������  |
	//                   +--------------+

	// �� ���� Chunk�� ���� ������ ���ƹ���.
	// �������� ��� Free�� �Ϸ�Ǿ �ٽ� ChunkPool�� ��ȯ�Ǳ� �⵵�ؾߵȴ�..��� ���� ������. �� �κ��� ���� ����.


	template <typename T>
	class TLSArrayMemoryPool
	{
	public:
		// �� �޸𸮵��� _chunkPool���� new�� ���� �Ҵ�ȴ�.
		class MemoryChunk
		{
		public:
			struct DataNode
			{
				T data;
				MemoryChunk* owner;
			};

			// ���� PlacementNew �ɼ��� false�� ����ȵ�.
			MemoryChunk()
			{
				Initialize();
			}

			void Initialize()
			{
				_allocCount = 0;
				_freeCount = 0;
			}

			T* Alloc()
			{
				DataNode* dataNode = &_chunkData[_allocCount++];
				dataNode->owner = this;

				return reinterpret_cast<T*>(dataNode);
			}

			bool IsFull() const { return _allocCount == capacity; }

			// Free�� ���� �۾��� ����. ���� Increase ���ָ� ��. 
			// Chunk�� Free�Ǵ� ������ �Ҵ��� �޸�Ǯ�� �� ������ �ݳ��Ǿ��� �� ���̴�.
			bool FreeNCheckFull() { return InterlockedIncrement(&_freeCount) == capacity; }
#ifdef MYLOG
		public:
#else
		private:
#endif
			inline static constexpr DWORD capacity = 3000;
			DWORD _allocCount;
			alignas(64) volatile DWORD _freeCount;
			DataNode _chunkData[capacity];
		};

		TLSArrayMemoryPool(bool bPlacementNew = false) : _chunkPool(0, true, true), _bPlacementNew(bPlacementNew)
		{
		}
	public:
		T* Alloc()
		{
#ifdef MYLOG
			ULONGLONG index = InterlockedIncrement64((LONGLONG*)&order) % logSize;
			logs[index].allocPtr = nullptr;
#endif // MYLOG

			// �� ûũǮ�� �������� �ʴ´ٸ� ûũ Ǯ�� �ϳ� ���´�.
			if (nullptr == _threadLocalChunk)
			{
				_threadLocalChunk = _chunkPool.Alloc();
				_threadLocalChunk->Initialize();
#ifdef MYLOG
				logs[index].allocPtr = _threadLocalChunk;
#endif // MYLOG

			}

#ifdef MYLOG
			void* cur = _threadLocalChunk;

			logs[index].job = 0x11;
			logs[index].threadId = GetCurrentThreadId();
			logs[index].curPtr = cur;
			logs[index].freePtr = nullptr;
			logs[index].allocCount = _threadLocalChunk->_allocCount;
			logs[index].freeCount = _threadLocalChunk->_freeCount;
#endif // MYLOG

			T* data = _threadLocalChunk->Alloc();

			// ���⼭ �ٽ� Ȯ���� ���ִ� ����.
			// �� ���� null check �ڵ忡�� ���� ������ ������ ����
			// Alloc �� free�� �� ���� ��ȯ <-- [ �߰� �ð��� �ٸ� �����尡 ���� pool�� alloc�� �ʱ�ȭ ] --> �ٽ� Alloc üũ
			//                                             ��
			//                                     --------+ �� �ð��� �ٸ� �����尡 pool�� �޾ư���
			// ���� �����尡 �ϳ��� pool�� �����ϴ� ��Ȳ�� �߻��Ѵ�.
			// �װ��� �ذ��� ���� �Ҵ� ������ ��尡 ������ �ٷ� pool�� �������ֵ��� �Ѵ�.

			if (true == _threadLocalChunk->IsFull())
			{
				_threadLocalChunk = _chunkPool.Alloc();

				_threadLocalChunk->Initialize();
			}

			if (true == _bPlacementNew)
				new (data) T();

			return data;
		}


		void Free(T* dataPtr)
		{
			if (true == _bPlacementNew)
				dataPtr->~T();

			//typename MemoryChunk::DataNode* dataNode = reinterpret_cast<MemoryChunk::DataNode*>(dataPtr);
			MemoryChunk* owner = (reinterpret_cast<MemoryChunk::DataNode*>(dataPtr))->owner;

			//MemoryChunk* owner = dataNode->owner;

			if (true == owner->FreeNCheckFull())
			{
				_chunkPool.Free(owner);

#ifdef MYLOG
				ULONGLONG index = InterlockedIncrement64((LONGLONG*)&order) % logSize;
				logs[index].job = 0x22;
				logs[index].allocPtr = nullptr;
				logs[index].threadId = GetCurrentThreadId();
				logs[index].freePtr = owner;
				logs[index].curPtr = nullptr;
				logs[index].allocCount = owner->_allocCount;
				logs[index].freeCount = owner->_freeCount;

#endif // DEBUG

			}
		}


		void SetDebugBreak()
		{
			int* a = nullptr;

			*a = 3;
		}

		// ���� ���� �ڽ��� ����ϴ� ûũ�� Ǯ�� �ݳ� (��������)
		void DeallocateChunk()
		{
			if (nullptr != _threadLocalChunk)
			{
				_chunkPool.Free(_threadLocalChunk);
				_threadLocalChunk = nullptr;
			}
		}
	private:
#ifdef MYLOG
		struct MemLog
		{
			byte job; // 0x11 Alloc, 0x22 Free
			uint threadId;

			void* allocPtr;
			void* freePtr;
			void* curPtr;

			uint allocCount;
			uint freeCount;
		};
		inline static constexpr ULONGLONG logSize = 100000;
		MemLog logs[logSize];
		alignas(64) volatile ULONGLONG order = 0;
#endif
		LockFreeMemoryPool<MemoryChunk> _chunkPool;
		bool _bPlacementNew;
		inline static thread_local MemoryChunk* _threadLocalChunk = nullptr;

	};
}
