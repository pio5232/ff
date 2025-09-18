#ifndef CUSTOM_LOCK_FREE_MEMORY_POOL_
#define CUSTOM_LOCK_FREE_MEMORY_POOL_

#include <Windows.h>
#include <functional>
// DEBUG 모드에서는 STACK_LOG를 무조건 출력하도록 한다.
#ifdef FS// _DEBUG
#ifndef STACK_LOG
#define STACK_LOG
#endif 
#endif // DEBUG

//#define STACK_LOG
// RELEASE 모드에서 STACK_LOG를 출력하고 싶으면 STACK_LOG를 DEFINE하도록 한다.


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
			// LOG에 어떤 내용이 들어가야 할 지 생각.
			char _job; //0x66 push , 0xff pop
			DWORD _threadId; // thread id

			// Node<T>*
			LPVOID _newTop; // push에서는 새로 연결되는 노드, pop에서는 top->next였던 노드
			LPVOID _prevTop; // push에서 top->next, pop에서는 delete될 노드.
			//LONG _size;
			LPVOID _newNext; // Pop할 때 넣기 전 top의 next 사본.
			ULONG _seqNumber;
			// 이렇게하면 안된다.. static도 안되고.. 생각해보자
			//ULONG _prevLogIndex; // 저장 순서가 꼬이지 않게 하기 위해서 while->끝나고 바로 index에 대한 log interlock -> 에러가 났을 때 전에 성공한 인덱스를 알 수 없어서 저장한다.
		};
#endif
	public:
		// 초기에 할당받은 큰 크기의 값을 단위로 쪼개서 리스트에 넣는다.
		// 일단 MemoryPool에서 Placement New의 호출은 기본적으로 하지 않도록 한다.
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

			// [Index 번호] [Node 주소] 합친 데이터.

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
					// 리스트가 비었으므로 루프를 종료합니다.
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

		// 내 리스트의 &T (시작 주소) 반환
		// 추가를 허용하지 않을 경우 공간이 부족하면 nullptr을 리턴한다.
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
				// Top 세팅
				tmpTopNode = m_pTopNode;

				// 더 이상 줄 노드가 없으면 새로 만들어서 준다.
				if (!tmpTopNode)
				{
					//// 추가를 허락하지 않음.
					//if (!_isExtraMode)
					//	return nullptr;

					// 노드 생성
					// 
					tmpTopNode = MallocNode();

					InterlockedIncrement(&m_lBlockCount);
					// 생성한 노드의 데이터 부분을 던진다.
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
			//이상하다면 조치를 취해야함.
			// return을 false로 할 수 있도록 한다.
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
				// 메모리 부족!! Crash내도록 한다.
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
		alignas(64) LONG m_lUseCount; // Alloc으로 나눠준 노드 수 
		alignas(64) LONG64 m_llSeqNumber;
		alignas(64) Node* m_pTopNode; // Top 부분

		// 초기 생성에 생성자를 생성, 종료할 때 소멸자를 호출할 것인지, Alloc/Free에서 PlacementNew를 통한 생성 / 소멸자 호출을 할 것인지 설정하는 플래그
		// true -> Alloc/Free에서 PlacementNew를 통한 생성 / 소멸자 호출 , false ->  풀 생성자/소멸자에서 1번씩만 호출

		//bool _isExtraMode;// 노드가 부족할 때 추가 여부를 설정. 일단 이것도 제외하자.

#ifdef STACK_LOG
		volatile ULONG m_ulLogIdx = 0;
		const static ULONG m_ulLogMax = 500000;
		//POOL_STACK_LOG m_pLog[m_ulLogMax];
		POOL_STACK_LOG* m_pLog;
#endif // STACK_LOG
	};

}

#endif // 

