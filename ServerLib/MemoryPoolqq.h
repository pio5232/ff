#pragma once

#include <queue>
#include <vector>

namespace jh_utility
{
	enum : uint
	{
		NODE_ALIGN_SIZE = 16,
		POOL_ALIGN_SIZE = 32,

		POOL_SIZE_LEVEL_1 = 32,
		POOL_SIZE_LEVEL_2 = 128,
		POOL_SIZE_LEVEL_3 = 256,

		POOL_COUNT_LEVEL_1 = 1024 / POOL_SIZE_LEVEL_1,
		POOL_COUNT_LEVEL_2 = 1024 / POOL_SIZE_LEVEL_2,
		POOL_COUNT_LEVEL_3 = 2048 / POOL_SIZE_LEVEL_3,

		POOL_COUNT_TO_LEVEL_2 = POOL_COUNT_LEVEL_1 + POOL_COUNT_LEVEL_2,
		POOL_COUNT_TO_LEVEL_3 = POOL_COUNT_LEVEL_1 + POOL_COUNT_LEVEL_2 + POOL_COUNT_LEVEL_3,
		
		EXTRA_CHUNK_MOVE_SIZE = 200,
		MEMORY_ALLOC_SIZE = 4000, // 
		MAX_ALLOC_SIZE = 4096
	};

	enum : MemoryGuard
	{
		GUARD_VALUE = 0xaabbccdd,
	};


	class PoolInfo
	{
	public:
		static inline uint GetPoolIndex(uint size)
		{
			// if vs O(1) hash 비교 필요
			if (size <= 1024)
			{
				if (size % POOL_SIZE_LEVEL_1 == 0)
					return size / POOL_SIZE_LEVEL_1;

				return (size + POOL_SIZE_LEVEL_1) / POOL_SIZE_LEVEL_1 - 1;
			}

			size -= 1024;
			if (size <= 1024)
			{
				if (size % POOL_SIZE_LEVEL_2 == 0)
					return POOL_COUNT_LEVEL_1 + size / POOL_SIZE_LEVEL_2 -1;

				return POOL_COUNT_LEVEL_1 + (size + POOL_SIZE_LEVEL_2) / POOL_SIZE_LEVEL_2 - 1;
			}

			size -= 1024;

			if (size % POOL_SIZE_LEVEL_3 == 0)
				return POOL_COUNT_TO_LEVEL_2 + size / POOL_SIZE_LEVEL_3 -1;

			return POOL_COUNT_TO_LEVEL_2 + (size + POOL_SIZE_LEVEL_3) / POOL_SIZE_LEVEL_3 - 1;
		}

		static constexpr uint consArray[POOL_COUNT_TO_LEVEL_3] = { 32 * 1, 32 * 2, 32 * 3, 32 * 4, 32 * 5, 32 * 6, 32 * 7, 32 * 8, 32 * 9,
			32 * 10, 32 * 11, 32 * 12, 32 * 13, 32 * 14, 32 * 15, 32 * 16, 32 * 17, 32 * 18, 32 * 19,
			32 * 20, 32 * 21, 32 * 22, 32 * 23, 32 * 24, 32 * 25, 32 * 26, 32 * 27, 32 * 28, 32 * 29,
			32 * 30, 32 * 31, 32 * 32,
			1024 + 128 * 1, 1024 + 128 * 2, 1024 + 128 * 3, 1024 + 128 * 4, 1024 + 128 * 5, 1024 + 128 * 6, 1024 + 128 * 7, 1024 + 128 * 8,
			2048 + 256 * 1, 2048 + 256 * 2, 2048 + 256 * 3, 2048 + 256 * 4, 2048 + 256 * 5, 2048 + 256 * 6, 2048 + 256 * 7, 2048 + 256 * 8
		};
	};
	/*--------------------------
		   MemoryProtector
	--------------------------*/
	class MemoryProtector // Use Header + Guard 
	{
	public:
		MemoryProtector(uint size);

		static void* Attach(void* ptr, uint size);

		static MemoryProtector* Detach(void* ptr);

		inline uint GetSize() const { return _size; }

		static constexpr uint _rightGuardSpace = sizeof(MemoryGuard) + sizeof(void*);
	private:
		uint _size; // header->size

		MemoryGuard _frontGuard; // 

	};

	/*-----------------
			Node
	------------------*/
	template <uint AllocSize>
	struct Node
	{
		char c[AllocSize]{};
		Node<AllocSize>* next;
	};


	struct NodeChunkBase
	{
		virtual ~NodeChunkBase() = 0;
	};
	/*-------------------
		  NodeChunk
	-------------------*/
	template <uint AllocSize>
	struct NodeChunk : public NodeChunkBase
	{
	public:
		using PoolNode = Node<AllocSize>;
		NodeChunk() : _head(nullptr), _size(0) {}
		~NodeChunk()
		{
			while (_head)
			{
				PoolNode* next = _head->next;

				_aligned_free(_head);

				_head = next;

				--_size;
			}

			if (_size == 0)
			{
				//std::cout << "NodeChunk Size is 0 ~ Destructor call successed\n";
			}
			else
			{
				std::cout << "NodeChunk Size is Not 0 - Error\n";
			}
		}
		inline uint GetSize() const { return _size; }

		void* Export() // == Alloc
		{
			if (!_head)
				return nullptr;

			PoolNode* ret = _head;
			_head = _head->next;

			--_size;
			return ret;
		}

		void Regist(void* ptr) // == Free
		{
			PoolNode* newTop = static_cast<PoolNode*>(ptr);
			newTop->next = _head;
			_head = newTop;

			++_size;
		}


	private:
		inline PoolNode* GetHead() { return _head; }
		PoolNode* _head;
		uint _size;
	};


	/*-----------------------------------
		   IntegrationChunkManager
	-----------------------------------*/
	class IntegrationChunkManager // has Chunk Pool
	{
	public:
		static IntegrationChunkManager& GetInstance()
		{
			static IntegrationChunkManager instance;
			return instance;
		}

		// TODO_추가
		uint GetPoolSize(uint allocSize) const { return _chunkKeepingQ[PoolInfo::GetPoolIndex(allocSize)].size(); } // 추가
		void RegistChunk(uint poolSize, void* nodePtr);
		void* GetNodeChunk(uint poolSize);
		IntegrationChunkManager(const IntegrationChunkManager&) = delete;
		IntegrationChunkManager& operator = (const IntegrationChunkManager&) = delete;
	private:
		IntegrationChunkManager() { InitializeSRWLock(&_playerLock); }
		~IntegrationChunkManager();
		SRWLOCK _playerLock;
		std::queue<void*> _chunkKeepingQ[POOL_COUNT_TO_LEVEL_3];
	};



	class MemoryPoolBase
	{
	public:
		MemoryPoolBase() {}
		virtual ~MemoryPoolBase() = 0;

		virtual void* Alloc() abstract = 0;
		virtual void Free(void* ptr) abstract = 0;
	};

	/*---------------------------------------
		Memory Pool (memoryProtector + size)
	---------------------------------------*/
	template <uint AllocSize> // pool Allocation Size
	class MemoryPool final : public MemoryPoolBase
	{

	public:
		MemoryPool(uint managedCnt = MEMORY_ALLOC_SIZE) : _managedCount(managedCnt)
		{
			MakeChunk(_pInnerChunk);
			MakeChunk(_pExtraChunk);

			for (uint i = 0; i < _managedCount; i++)
			{
				_pInnerChunk->Regist(_aligned_malloc(AllocSize, 64));
			}

		}
		virtual ~MemoryPool() override
		{
 			DeleteChunk(_pInnerChunk);
			DeleteChunk(_pExtraChunk);
		}
		inline void MakeChunk(NodeChunk<AllocSize>*& nodeChunk)
		{
			nodeChunk = static_cast<NodeChunk<AllocSize>*>(_aligned_malloc(sizeof(NodeChunk<AllocSize>), NODE_ALIGN_SIZE));
			new (nodeChunk) NodeChunk<AllocSize>();
		}

		void* Alloc() override
		{
			if (_pInnerChunk->GetSize() > 0)
			{
				return _pInnerChunk->Export();
			}

			if (_pExtraChunk->GetSize() > 0)
			{
				return _pExtraChunk->Export();
			}

			if (IntegrationChunkManager::GetInstance().GetPoolSize(AllocSize) > 0)
			{
				void* newNodeChunk = IntegrationChunkManager::GetInstance().GetNodeChunk(AllocSize);

				if (newNodeChunk != nullptr)
				{
					DeleteChunk(_pInnerChunk);
					_pInnerChunk = reinterpret_cast<NodeChunk<AllocSize>*>(newNodeChunk);
					return _pInnerChunk->Export();
				}
			}
			
			return _aligned_malloc(AllocSize, 64);
		}

		void Free(void* ptr) override
		{
			if (_pInnerChunk->GetSize() < _managedCount)
			{
				_pInnerChunk->Regist(ptr);
				return;
			}

			_pExtraChunk->Regist(ptr); // Managing NodeChunk, if (NodeChunk Size -> 100, Regist NodeChunk to Global Manager)
			if (_pExtraChunk->GetSize() > EXTRA_CHUNK_MOVE_SIZE)
			{
				IntegrationChunkManager::GetInstance().RegistChunk(AllocSize, _pExtraChunk);

				MakeChunk(_pExtraChunk);
				return;
			}
		}
	private:
		NodeChunk<AllocSize>* _pInnerChunk;
		NodeChunk<AllocSize>* _pExtraChunk;

		const uint _managedCount;
	};



	/*---------------------------------------
			 Pool Manager ( 1 per Thread )
	---------------------------------------*/

	class PoolManager final
	{
	public:
		void* Alloc(uint size);
		void Free(void* ptr);

		PoolManager(const PoolManager&) = delete;
		PoolManager& operator = (const PoolManager&) = delete;

		PoolManager();
		~PoolManager();

	private:
		MemoryPoolBase* _pools[POOL_COUNT_TO_LEVEL_3];
	};

	// -------------------- Global ------------------------
	inline void DeleteChunk(NodeChunkBase* nodeChunkBase)
	{
		nodeChunkBase->~NodeChunkBase();
		_aligned_free(nodeChunkBase);
	}

}
	extern thread_local jh_utility::PoolManager poolMgr;

	class MemoryPool
	{
	};
