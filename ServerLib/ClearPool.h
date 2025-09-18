#pragma once

#include <Windows.h>
#include <functional>


namespace jh_utility
{
	template <typename T>
	class ClearPool
	{
		struct alignas(64) Node
		{
			T data;
			ULONGLONG guard = Node::staticGuardValue;
			Node* next;
			inline static constexpr ULONGLONG staticGuardValue = 0x1122334455667788;
		};

	public:
		using InitFunc = void(T::*)();
		// �ʱ⿡ �Ҵ���� ū ũ���� ���� ������ �ɰ��� ����Ʈ�� �ִ´�.
		// �ϴ� MemoryPool���� Placement New�� ȣ���� �⺻������ ���� �ʵ��� �Ѵ�.
		ClearPool(LONG capacity = 0, InitFunc func = nullptr) : m_lBlockCount(0), m_lUseCount(0),
			m_pTopNode(nullptr), m_llSeqNumber(0), m_pInitFunc(func)
		{
			for (int i = 0; i < capacity; i++)
				CreateNode();

		}

		void CreateNode()
		{
			Node* newNode = MallocNode();

			Node* tmpTop;

			ULONG seqNum = static_cast<ULONG>(InterlockedIncrement64(&m_llSeqNumber) % m_ullIndexMax);

			do
			{
				tmpTop = m_pTopNode;

				newNode->next = tmpTop;

			} while (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), reinterpret_cast<PVOID>(((ULONGLONG)newNode | ((ULONGLONG)seqNum) << 47)), tmpTop) != tmpTop);

			InterlockedIncrement(&m_lBlockCount);

		}

		~ClearPool()
		{
			// ����.
			while (1)
			{
				Node* puretopNode = (Node*)((ULONGLONG)m_pTopNode & 0x00007fffffffffff);

				if (nullptr == puretopNode)
					break;

				//Node* delNode = puretopNode;

				m_pTopNode = puretopNode->next;

				delete (puretopNode);

				_aligned_free(puretopNode);

				InterlockedDecrement(&m_lBlockCount);
			}

			if (m_lBlockCount != 0)
			{
				// Log
				DebugBreak();
			}
		}

		// �� ����Ʈ�� &T (���� �ּ�) ��ȯ
		// �߰��� ������� ���� ��� ������ �����ϸ� nullptr�� �����Ѵ�.
		T* Alloc()  // = POP;
		{
			T* _pRet;

			Node* tmpTopNode;
			Node* nextNode = nullptr;
			Node* pureTmpNode;

			InterlockedIncrement(&m_lUseCount);

			do
			{
				// Top ����
				tmpTopNode = m_pTopNode;

				// �� �̻� �� ��尡 ������ ���� ���� �ش�.
				if (!tmpTopNode)
				{
					tmpTopNode = MallocNode();

					InterlockedIncrement(&m_lBlockCount);
					// ������ ����� ������ �κ��� ������.
					_pRet = reinterpret_cast<T*>(tmpTopNode);

					break;
				}
				pureTmpNode = (Node*)((ULONGLONG)tmpTopNode & 0x00007fffffffffff);
				nextNode = pureTmpNode->next;

				_pRet = reinterpret_cast<T*>(pureTmpNode);

			} while (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), nextNode, tmpTopNode) != tmpTopNode);


			return _pRet;
		}

		bool Free(T* pData) // PUSH
		{
			Node* myNode = reinterpret_cast<Node*>(pData);

			Node* tmpTopNode;
			ULONG seqNum = static_cast<ULONG>(InterlockedIncrement64(&m_llSeqNumber) % m_ullIndexMax);

			if (nullptr != m_pInitFunc)
				(pData->*m_pInitFunc)();

			do
			{
				tmpTopNode = m_pTopNode;

				myNode->next = tmpTopNode;

			} while (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), (PVOID)((ULONGLONG)myNode | ((ULONGLONG)seqNum << 47)), tmpTopNode) != tmpTopNode);

			InterlockedDecrement(&m_lUseCount);

			return true;
		}

		ULONG GetCreationCount() const
		{
			return m_lBlockCount;
		}

		ULONG GetAllocCount() const
		{
			return m_lUseCount;
		}


	private:

		inline Node* MallocNode()
		{
			Node* newNode = static_cast<Node*>(_aligned_malloc(sizeof(Node), 64));

			if (!newNode)
			{
				// �޸� ����!! Crash������ �Ѵ�.
				DebugBreak();
			}
			else
			{
				newNode->guard = Node::staticGuardValue;
				newNode->next = nullptr;

				new (reinterpret_cast<T*>(newNode)) T();
			}

			return newNode;
		}
		const InitFunc m_pInitFunc;
		const ULONGLONG m_ullIndexMax = 0x0000'0000'0002'0000;
		//0b00000000'00000000'00000000'00000000'00000000'00000001'11111111'11111111;// 131072; // 17 bit max

		alignas(64) LONG m_lBlockCount; // 
		alignas(64) LONG m_lUseCount; // Alloc���� ������ ��� �� 
		alignas(64) LONG64 m_llSeqNumber;
		alignas(64) Node* m_pTopNode; // Top �κ�
	};

}

