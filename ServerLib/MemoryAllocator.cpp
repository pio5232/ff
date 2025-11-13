#include "LibraryPch.h"
#include "MemoryAllocator.h"


jh_memory::MemoryAllocator::MemoryAllocator() : m_nodeStack{}, m_pPool{}
{

}

jh_memory::MemoryAllocator::~MemoryAllocator()
{
	if (nullptr == m_pPool)
		return;

	for (int i = 0; i < kPoolCount; i++)
	{
		NodeStack& stack = m_nodeStack[i];

		while (1)
		{
			// 스택이 빌 때까지 다음 노드를 꺼냅니다.
			Node* node = stack.Pop();

			if (nullptr == node)
				break;

			m_pPool[i]->TryPushNode(node);
		}
	}
}

void* jh_memory::MemoryAllocator::Alloc(size_t allocSize)
{
	MEMORY_POOL_PROFILE_FLAG;
	int poolIdx = poolTable[allocSize];

	void* allocedPointer = m_nodeStack[poolIdx].Pop();

	// 할당할 메모리가 없는 경우 L2에서 가져와서 다시 할당한다.
	if (nullptr == allocedPointer)
	{
		AcquireBlockFromPool(poolIdx);
		return m_nodeStack[poolIdx].Pop();
	}
	return allocedPointer;
}



void jh_memory::MemoryAllocator::Dealloc(void* ptr, size_t allocSize)
{
	MEMORY_POOL_PROFILE_FLAG;
	int poolIdx = poolTable[allocSize];

	NodeStack& nodeStack = m_nodeStack[poolIdx];
	// 해제한 메모리 반납한다.
	nodeStack.Push(static_cast<Node*>(ptr));

	// 일정 수량 이상이면 절반을 LEVEL 2에 반납한다.

	if (nodeStack.GetTotalCount() == (kNodeCountPerBlock * 2))
	{
		m_pPool[poolIdx]->TryPushBlock(nodeStack.m_pSubHead, kNodeCountPerBlock);
		nodeStack.m_pSubHead = nullptr;
		nodeStack.m_subCount = 0;

	}
}
