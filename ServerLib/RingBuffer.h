#pragma once

namespace jh_utility
{
#define RINGBUFFER_DEFAULT_SIZE 4000
	class RingBuffer
	{
	public:
		RingBuffer();
		RingBuffer(int iCapacity);
		~RingBuffer();
		bool Resize(int newCapacity);
		int GetCapacity(); // �� �뷮
		int GetUseSize(); // ��� �뷮
		int GetFreeSize(); // ���� �뷮

		int DirectEnqueueSize();
		int DirectDequeueSize();

		bool EnqueueRetBool(char* chpData, int iSize);
		bool DequeueRetBool(char* chpDest, int iSize);
		bool PeekRetBool(char* chpDest, int iSize);

		int Enqueue(char* chpData, int iSize);
		int Dequeue(char* chpDest, int iSize);
		int Peek(char* chpDest, int iSize);

		int MoveRear(int iSize);
		int MoveFront(int iSize);

		bool MoveRearRetBool(int iSize);
		bool MoveFrontRetBool(int iSize);

		void ClearBuffer(void);
		char* GetFrontBufferPtr();
		char* GetRearBufferPtr();
		char* GetStartBufferPtr();

		bool IsEmpty();
		bool IsFull();

		//inline void BufferLock() { AcquireSRWLockExclusive(&m_lock); }
		//inline void BufferUnlock() {ReleaseSRWLockExclusive(&m_lock);}
	private:
		int m_iCapacity;
		int m_iFront;
		int m_iRear;
		char* m_chpBuffer;
		//alignas(64) SRWLOCK m_lock;
	};
}
