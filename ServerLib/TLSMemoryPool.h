#pragma once

#include "LFMemoryPool.h"

//#define MYLOG

namespace jh_utility
{
	// 배열 형태의 TLS 메모리풀. ALLOC과 FREE에 대한 작업을 분리.
	// 이것을 사용할땐 capacity만큼 할당 -> 다른 Chunk로 바꾼다.
	// 다른 Chunk로 바뀐 상태에서 기존 데이터에 대한 해제 -> 각 스레드에서 접근해도 interlocke으로 Free Count만 늘려주고
	// 마지막에 해제한 스레드가 다시 스레드풀에 넣도록 한다.

	// 그러나 내가 생각했을 때 할당/해제에 대해서 한 스레드가 진행하지 않거나 마지막 할당 상황에서는
	// +------------------------------------------------------------+
	// | 이전 Chunk |    | 새로운 Chunk |
	// +------------+    +--------------+
	//                          ↑
	//                   +--------------+
	//                   |   내 스레드  |
	//                   +--------------+

	// 로 이전 Chunk에 대한 관리를 놓아버림.
	// 전적으로 모든 Free가 완료되어서 다시 ChunkPool에 반환되길 기도해야된다..라는 점이 있으며. 이 부분이 단점 같음.


	template <typename T>
	class TLSArrayMemoryPool
	{
	public:
		// 이 메모리들은 _chunkPool에서 new를 통해 할당된다.
		class MemoryChunk
		{
		public:
			struct DataNode
			{
				T data;
				MemoryChunk* owner;
			};

			// 현재 PlacementNew 옵션이 false라 적용안됨.
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

			// Free에 대한 작업이 없음. 단지 Increase 해주면 끝. 
			// Chunk가 Free되는 조건은 할당한 메모리풀이 다 쓰여서 반납되었을 때 뿐이다.
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

			// 내 청크풀이 존재하지 않는다면 청크 풀을 하나 얻어온다.
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

			// 여기서 다시 확인을 해주는 이유.
			// 저 위의 null check 코드에서 같이 진행을 시켰을 때는
			// Alloc 후 free로 꽉 차서 반환 <-- [ 중간 시간에 다른 스레드가 새로 pool을 alloc후 초기화 ] --> 다시 Alloc 체크
			//                                             ↑
			//                                     --------+ 이 시간에 다른 스레드가 pool을 받아가면
			// 여러 스레드가 하나의 pool을 공유하는 상황이 발생한다.
			// 그것의 해결을 위해 할당 가능한 노드가 없으면 바로 pool을 변경해주도록 한다.

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

		// 종료 직전 자신이 사용하던 청크를 풀에 반납 (해제위해)
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
