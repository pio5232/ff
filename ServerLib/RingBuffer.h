#pragma once

namespace jh_utility
{
#define RINGBUFFER_DEFAULT_SIZE 16384
	class RingBuffer
	{
	public:
		RingBuffer();
		RingBuffer(int iCapacity);
		~RingBuffer();
		bool Resize(int newCapacity);
		int GetCapacity(); // 총 용량
		int GetUseSize(); // 사용 용량
		int GetFreeSize(); // 남은 용량

		int DirectEnqueueSize();
		int DirectDequeueSize();

		bool EnqueueRetBool(char* chpData, int iSize);
		bool DequeueRetBool(char* chpDest, int iSize);
		bool PeekRetBool(char* chpDest, int iSize);
		 
		int Enqueue(char* chpData, int iSize);
		int Dequeue(char* chpDest, int iSize);
		int Peek(char* chpDest, int iSize);

		bool MoveRear(int iSize);
		bool MoveFront(int iSize);

		void ClearBuffer(void);
		char* GetFrontBufferPtr();
		char* GetRearBufferPtr();
		char* GetStartBufferPtr();

		bool IsEmpty();
		bool IsFull();

	private:
		int		m_iCapacity;
		int		m_iFront;
		int		m_iRear;
		char	* m_chpBuffer;
	};
}
