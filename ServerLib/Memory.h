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

            // �ʿ��ϸ� ����
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
        // ��� �ɰ���
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
        std::vector<char*> m_chunks; // �Ҵ�� ûũ���� �����ϱ� ���� ����Ʈ
    };


    // ����ڴ� MemoryAllocator�� ���
    class MemoryAllocator
    {
    private:
        enum
        {
            // ~1024���� 32����, ~2048���� 128����, ~4096���� 256����
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

            // [ũ��] [������]  ���� �� ������ ��ȯ
            return MemoryHeader::AttachHeader(header, ownerPool);
        }

        void Free(void* ptr)
        {
            MemoryHeader* header = MemoryHeader::DetachHeader(ptr);

            // header->ownerPool == nullptr -> Size�� max���� ũ��
            // header->ownerPool != nullptr -> �� �����ͷ� �ٷ� ��ȯ�ϸ� �ȴ�.

            if (header->m_pOwnerPool != nullptr)
            {
                header->m_pOwnerPool->Dealloc(header);
            }
            // nullptr -> ũ�Ⱑ max���� ū ��� malloc���� �Ҵ�޾ұ� ������ free�� �ٷ� ������Ų��.
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

    // malloc�� �����ϸ� crash
    void* rawPtr = allocator->Alloc(sizeof(T));

    T* t = new (rawPtr) T(std::forward<Args>(args)...);

    auto deleter = [allocator](T* obj)
        {
            obj->~T();

            allocator->Free(obj);
        };

    // raw �����Ϳ� �����ڸ� ��� -> ����
    return std::shared_ptr<T>(t, deleter);
}

PacketPtr MakeSharedBuffer(jh_memory::MemoryAllocator* allocator, size_t bufferSize);
