#ifndef CUSTOM_LOCK_FREE_MEMORY_POOL_
#define CUSTOM_LOCK_FREE_MEMORY_POOL_

#include <Windows.h>
#include <functional>
// DEBUG ��忡���� STACK_LOG�� ������ ����ϵ��� �Ѵ�.
#ifdef FS// _DEBUG
#ifndef STACK_LOG
#define STACK_LOG
#endif 
#endif // DEBUG

//#define STACK_LOG
// RELEASE ��忡�� STACK_LOG�� ����ϰ� ������ STACK_LOG�� DEFINE�ϵ��� �Ѵ�.


namespace jh_utility
{
	template <typename T>
	class LockFreeMemoryPool
	{
		struct alignas(64) Node
		{
			T data;
			ULONGLONG guard = Node::staticGuardValue;
			Node* next;
			inline static constexpr ULONGLONG staticGuardValue = 0x1122334455667788;
		};

#ifdef STACK_LOG
		struct POOL_STACK_LOG
		{
			// LOG�� � ������ ���� �� �� ����.
			char _job; //0x66 push , 0xff pop
			DWORD _threadId; // thread id

			// Node<T>*
			LPVOID _newTop; // push������ ���� ����Ǵ� ���, pop������ top->next���� ���
			LPVOID _prevTop; // push���� top->next, pop������ delete�� ���.
			//LONG _size;
			LPVOID _newNext; // Pop�� �� �ֱ� �� top�� next �纻.
			ULONG _seqNumber;
			// �̷����ϸ� �ȵȴ�.. static�� �ȵǰ�.. �����غ���
			//ULONG _prevLogIndex; // ���� ������ ������ �ʰ� �ϱ� ���ؼ� while->������ �ٷ� index�� ���� log interlock -> ������ ���� �� ���� ������ �ε����� �� �� ��� �����Ѵ�.
		};
#endif
	public:
		// �ʱ⿡ �Ҵ���� ū ũ���� ���� ������ �ɰ��� ����Ʈ�� �ִ´�.
		// �ϴ� MemoryPool���� Placement New�� ȣ���� �⺻������ ���� �ʵ��� �Ѵ�.
		LockFreeMemoryPool(LONG capacity = 0, bool bPlacementNew = false) : m_lBlockCount(0), m_lUseCount(0),
			m_pTopNode(nullptr),/* _isExtraMode(isExtraMode),*/ m_llSeqNumber(0), m_bPlacementFlag(bPlacementNew)
		{
#ifdef  STACK_LOG
			m_pLog = new POOL_STACK_LOG[m_ulLogMax];
#endif //  _STACK_LOG

			for (int i = 0; i < capacity; i++)
				CreateNode();


		}

		void CreateNode()
		{
			Node* newNode = MallocNode();

			Node* tmpTop;

			ULONG seqNum = static_cast<ULONG>(InterlockedIncrement64(&m_llSeqNumber) % m_ullIndexMax);

			// [Index ��ȣ] [Node �ּ�] ��ģ ������.

			//PVOID EncNode = reinterpret_cast<PVOID>((ULONGLONG)newNode | ((InterlockedIncrement64(&m_llSeqNumber) % m_ullIndexMax) << 47)));

			do
			{
				tmpTop = m_pTopNode;

				newNode->next = tmpTop;

				//} while (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&m_pTopNode), EncNode, tmpTop) != tmpTop);
			} while (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), reinterpret_cast<PVOID>(((ULONGLONG)newNode | ((ULONGLONG)seqNum) << 47)), tmpTop) != tmpTop);

			/////////////////////////////////////////////////////////////////////////////////
			// InterlockedCompareExchangePointer (Destination, Exchange, Comperand)
			//
			// if (Destination == Comperand)
			// {			
			//	   Temp = Destination
			//     Destination = Exchange
			// }
			// return Temp;
			/////////////////////////////////////////////////////////////////////////////////

#ifdef STACK_LOG
			ULONG logIndex = InterlockedIncrement(&m_ulLogIdx) % m_ulLogMax;
#endif // STACK_LOG

			InterlockedIncrement(&m_lBlockCount);

#ifdef STACK_LOG
			// == LOG ==
			m_pLog[logIndex]._job = 0x11;
			m_pLog[logIndex]._threadId = GetCurrentThreadId();
			m_pLog[logIndex]._newTop = (PVOID)((ULONGLONG)newNode | ((ULONGLONG)seqNum) << 47);
			m_pLog[logIndex]._prevTop = tmpTop;
			m_pLog[logIndex]._newNext = nullptr;
			m_pLog[logIndex]._seqNumber = seqNum;
#endif
		}

		~LockFreeMemoryPool()
		{

			while (1)
			{
				Node* topNode = m_pTopNode;
				if (topNode == nullptr)
				{
					// ����Ʈ�� ������Ƿ� ������ �����մϴ�.
					break;
				}

				Node* pureTopNode = (Node*)((ULONGLONG)topNode & 0x00007fffffffffff);
				Node* nextNode = pureTopNode->next;

				if (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), nextNode, topNode) == topNode)
				{
					_aligned_free(pureTopNode);
					InterlockedDecrement(&m_lBlockCount);
				}
			}


			if (m_lBlockCount != 0)
			{
				// Log
				DebugBreak();
			}

#ifdef STACK_LOG
			if (nullptr != m_pLog)
			{
				delete[] m_pLog;
				m_pLog = nullptr;
			}
#endif // STACK_LOG


		}

		// �� ����Ʈ�� &T (���� �ּ�) ��ȯ
		// �߰��� ������� ���� ��� ������ �����ϸ� nullptr�� �����Ѵ�.
		template<typename... Args>
		T* Alloc(Args&&... args)  // = POP;
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
					//// �߰��� ������� ����.
					//if (!_isExtraMode)
					//	return nullptr;

					// ��� ����
					// 
					tmpTopNode = MallocNode();

					InterlockedIncrement(&m_lBlockCount);
					// ������ ����� ������ �κ��� ������.
					_pRet = reinterpret_cast<T*>(tmpTopNode);

#ifdef STACK_LOG
					pureTmpNode = tmpTopNode;
					LONG logIndex = InterlockedIncrement(&m_ulLogIdx) % m_ulLogMax;

					// -- LOGGING --
					m_pLog[logIndex]._job = 0xff;
					m_pLog[logIndex]._threadId = GetCurrentThreadId();
					m_pLog[logIndex]._prevTop = tmpTopNode;
					m_pLog[logIndex]._newTop = nullptr;
					m_pLog[logIndex]._newNext = nullptr;
					m_pLog[logIndex]._seqNumber = (ULONG)((ULONGLONG)tmpTopNode >> 47);
#endif
					break;
				}
				pureTmpNode = (Node*)((ULONGLONG)tmpTopNode & 0x00007fffffffffff);
				nextNode = pureTmpNode->next;
				//_pRet = &pureTmpNode->data;
				_pRet = reinterpret_cast<T*>(pureTmpNode);


			} while (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), nextNode, tmpTopNode) != tmpTopNode);
#ifdef STACK_LOG
			LONG logIndex = InterlockedIncrement(&m_ulLogIdx) % m_ulLogMax;

			// -- LOGGING --
			m_pLog[logIndex]._job = 0x33;
			m_pLog[logIndex]._threadId = GetCurrentThreadId();
			m_pLog[logIndex]._prevTop = pureTmpNode;
			m_pLog[logIndex]._newTop = nextNode;
			m_pLog[logIndex]._newNext = pureTmpNode->next;
			m_pLog[logIndex]._seqNumber = (ULONG)((ULONGLONG)tmpTopNode >> 47);
#endif					
			if (true == m_bPlacementFlag)
				new (_pRet) T(std::forward<Args>(args)...);

			return _pRet;
		}

		bool Free(T* pData) // PUSH
		{
			//�̻��ϴٸ� ��ġ�� ���ؾ���.
			// return�� false�� �� �� �ֵ��� �Ѵ�.
			Node* myNode = reinterpret_cast<Node*>(pData);

			Node* tmpTopNode;
			ULONG seqNum = static_cast<ULONG>(InterlockedIncrement64(&m_llSeqNumber) % m_ullIndexMax);

			//PVOID EncNode = reinterpret_cast<PVOID>((ULONGLONG)newNode | ((InterlockedIncrement64(&m_llSeqNumber) % m_ullIndexMax) << 47)));

			if (true == m_bPlacementFlag)
			{
				pData->~T();
			}
			
			do
			{
				tmpTopNode = m_pTopNode;

				myNode->next = tmpTopNode;

			} while (InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pTopNode), (PVOID)((ULONGLONG)myNode | ((ULONGLONG)seqNum << 47)), tmpTopNode) != tmpTopNode);

#ifdef STACK_LOG
			ULONG logIndex = InterlockedIncrement(&m_ulLogIdx) % m_ulLogMax;
#endif	
			InterlockedDecrement(&m_lUseCount);

#ifdef STACK_LOG
			// == LOG ==
			m_pLog[logIndex]._job = 0x44;
			m_pLog[logIndex]._threadId = GetCurrentThreadId();
			m_pLog[logIndex]._newTop = myNode;
			m_pLog[logIndex]._prevTop = myNode->next;
			m_pLog[logIndex]._newNext = nullptr;
			m_pLog[logIndex]._seqNumber = seqNum;
#endif
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
			}

			return newNode;
		}
		const ULONGLONG m_ullIndexMax = 0x0000'0000'0002'0000;
		bool m_bPlacementFlag;
		//0b00000000'00000000'00000000'00000000'00000000'00000001'11111111'11111111;// 131072; // 17 bit max

		alignas(64) LONG m_lBlockCount; // 
		alignas(64) LONG m_lUseCount; // Alloc���� ������ ��� �� 
		alignas(64) LONG64 m_llSeqNumber;
		alignas(64) Node* m_pTopNode; // Top �κ�

		// �ʱ� ������ �����ڸ� ����, ������ �� �Ҹ��ڸ� ȣ���� ������, Alloc/Free���� PlacementNew�� ���� ���� / �Ҹ��� ȣ���� �� ������ �����ϴ� �÷���
		// true -> Alloc/Free���� PlacementNew�� ���� ���� / �Ҹ��� ȣ�� , false ->  Ǯ ������/�Ҹ��ڿ��� 1������ ȣ��

		//bool _isExtraMode;// ��尡 ������ �� �߰� ���θ� ����. �ϴ� �̰͵� ��������.

#ifdef STACK_LOG
		volatile ULONG m_ulLogIdx = 0;
		const static ULONG m_ulLogMax = 500000;
		//POOL_STACK_LOG m_pLog[m_ulLogMax];
		POOL_STACK_LOG* m_pLog;
#endif // STACK_LOG
	};

}

#endif // 

