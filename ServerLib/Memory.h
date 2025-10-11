#pragma once

#include <cassert>
#include "MemoryHeader.h"

namespace jh_memory
{
    class MemoryPool
    {
    public:

        MemoryPool(size_t blockSize) : m_blockSize(blockSize), m_pTopNode(nullptr)
        {
            InitializeSRWLock(&m_lock);

            m_chunks.reserve(s_defaultReserveSize);
        }

        ~MemoryPool()
        {
            for (char* chunk : m_chunks)
            {
                delete[] chunk;
            }
        }

        void* Alloc()
        {
            SRWLockGuard lockGuard(&m_lock);

            // 필요하면 생성
            if (m_pTopNode == nullptr)
            {
                AllocateNewChunk();
            }

            Node* top = m_pTopNode;
            m_pTopNode = top->next;
            return top;
        }

        void Dealloc(void* ptr)
        {
            SRWLockGuard lockGuard(&m_lock);

            Node* node = static_cast<Node*>(ptr);
            node->next = m_pTopNode;
            m_pTopNode = node;
        }

    private:
        struct Node
        {
            Node* next;
        };

        static constexpr int s_chunkSize = 128;
        static constexpr int s_defaultReserveSize = 20;
        // 노드 쪼개기
        void AllocateNewChunk()
        {
            char* newChunk = new char[m_blockSize * s_chunkSize];
            m_chunks.push_back(newChunk);

            for (int i = 0; i < s_chunkSize; ++i)
            {
                Node* node = reinterpret_cast<Node*>(newChunk + (i * m_blockSize));
                node->next = m_pTopNode;
                m_pTopNode = node;
            }
        }

        SRWLOCK m_lock;
        size_t m_blockSize;
        Node* m_pTopNode;
        std::vector<char*> m_chunks; // 할당된 청크들을 관리하기 위한 리스트
    };


    // 사용자는 MemoryAllocator를 사용
    class MemoryAllocator
    {
    private:
        enum
        {
            // ~1024까지 32단위, ~2048까지 128단위, ~4096까지 256단위
            POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256),
            MAX_ALLOC_SIZE = 4096
        };

    public:
        MemoryAllocator()
        {
            int size = 0;
            int tableIndex = 0;

            for (size = 32; size < 1024; size += 32)
            {
                MemoryPool* pool = new MemoryPool(size);
                m_pools.push_back(pool);

                while (tableIndex <= size)
                {
                    m_poolTable[tableIndex] = pool;
                    tableIndex++;
                }
            }

            for (; size < 2048; size += 128)
            {
                MemoryPool* pool = new MemoryPool(size);
                m_pools.push_back(pool);

                while (tableIndex <= size)
                {
                    m_poolTable[tableIndex] = pool;
                    tableIndex++;
                }
            }

            for (; size <= 4096; size += 256)
            {
                MemoryPool* pool = new MemoryPool(size);
                m_pools.push_back(pool);

                while (tableIndex <= size)
                {
                    m_poolTable[tableIndex] = pool;
                    tableIndex++;
                }
            }
        }

        ~MemoryAllocator()
        {
            for (MemoryPool* pool : m_pools)
            {
                delete pool;
            }
        }

        void* Alloc(size_t size)
        {
            size_t totalSize = size + sizeof(MemoryHeader);

            MemoryHeader* header = nullptr;
            MemoryPool* ownerPool;

            if (totalSize > MAX_ALLOC_SIZE)
            {
                header = static_cast<MemoryHeader*>(malloc(totalSize));

                if (header == nullptr)
                    jh_utility::CrashDump::Crash();

                ownerPool = nullptr;
            }
            else
            {
                ownerPool = m_poolTable[totalSize];
                header = static_cast<MemoryHeader*>(ownerPool->Alloc());
            }

            // [크기] [포인터]  저장 후 포인터 반환
            return MemoryHeader::AttachHeader(header, ownerPool);
        }

        void Free(void* ptr)
        {
            MemoryHeader* header = MemoryHeader::DetachHeader(ptr);

            // header->ownerPool == nullptr -> Size가 max보다 크다
            // header->ownerPool != nullptr -> 그 포인터로 바로 반환하면 된다.

            if (header->m_pOwnerPool != nullptr)
            {
                header->m_pOwnerPool->Dealloc(header);
            }
            // nullptr -> 크기가 max보다 큰 경우 malloc으로 할당받았기 때문에 free로 바로 해제시킨다.
            else
            {
                free(header);
            }
        }

    private:

        std::vector<MemoryPool*> m_pools;
        MemoryPool* m_poolTable[MAX_ALLOC_SIZE + 1];
    };
}

namespace jh_utility
{
    class SerializationBuffer;
}


template<typename T, typename... Args>
std::shared_ptr<T> MakeShared(jh_memory::MemoryAllocator* allocator, Args&&... args)
{
    if (allocator == nullptr)
        return nullptr;

    // malloc에 실패하면 crash
    void* rawPtr = allocator->Alloc(sizeof(T));

    T* t = new (rawPtr) T(std::forward<Args>(args)...);

    auto deleter = [allocator](T* obj)
        {
            obj->~T();

            allocator->Free(obj);
        };

    // raw 포인터와 삭제자를 등록 -> 생성
    return std::shared_ptr<T>(t, deleter);
}

PacketPtr MakeSharedBuffer(jh_memory::MemoryAllocator* allocator, size_t bufferSize);
