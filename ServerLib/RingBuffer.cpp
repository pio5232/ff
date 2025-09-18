#include "LibraryPch.h"
#include <iostream>
#include "RingBuffer.h"

using namespace jh_utility;

jh_utility::RingBuffer::RingBuffer() :m_iCapacity(RINGBUFFER_DEFAULT_SIZE), m_iFront(0), m_iRear(0)
{
	m_chpBuffer = new char[m_iCapacity];
	//InitializeSRWLock(&m_lock);
}


RingBuffer::RingBuffer(int iCapacity) : m_iFront(0), m_iRear(0)
{
	m_iCapacity = iCapacity;
	m_chpBuffer = new char[m_iCapacity];

	//InitializeSRWLock(&m_lock);
}

RingBuffer::~RingBuffer()
{
	delete[] m_chpBuffer;
}

// resize는 무조건 큰 사이즈로만.
bool RingBuffer::Resize(int newCapacity)
{
	if (newCapacity < m_iCapacity)
		return false;

	char* _tmpBuffer = new char[newCapacity];

	int _iBufferSize = GetUseSize();
	bool bRet = Dequeue(_tmpBuffer, _iBufferSize);

	delete m_chpBuffer;

	// 새로운 버퍼에 대한 포인터 재구성.
	m_chpBuffer = _tmpBuffer;

	m_iCapacity = newCapacity;
	m_iFront = 0;
	m_iRear = _iBufferSize;

	return bRet;
}

int RingBuffer::GetCapacity()
{
	return m_iCapacity;
}

int RingBuffer::GetUseSize()
{
	if (m_iFront == (m_iRear + 1) % m_iCapacity)
		return m_iCapacity - 1;

	if (m_iRear >= m_iFront)
		return m_iRear - m_iFront;

	else
		return m_iCapacity - (m_iFront - m_iRear);

}

int RingBuffer::GetFreeSize()
{
	if (m_iFront == (m_iRear + 1) % m_iCapacity)
		return 0;

	else if (m_iRear >= m_iFront)
		return m_iCapacity - (m_iRear - m_iFront) - 1;
	else
		return m_iFront - m_iRear - 1;
}

int RingBuffer::DirectEnqueueSize() // 
{
	if (m_iFront == (m_iRear + 1) % m_iCapacity)
		return 0;

	if (m_iRear >= m_iFront)
	{
		if (m_iFront == 0)
			return m_iCapacity - m_iRear - 1;

		return m_iCapacity - m_iRear;
	}
	else
		return m_iFront - m_iRear - 1;
}

int RingBuffer::DirectDequeueSize()
{
	if (m_iRear >= m_iFront)
		return m_iRear - m_iFront;

	return m_iCapacity - m_iFront;
}
// =============================================

// 내 정책 -> 링버퍼에 들어갈 수 있는 크기가 아니면 넣거나 꺼내지 못하도록 한다.
// 리턴 값은 성공 또는 실패로 지정한 사이즈를 넣거나 꺼내는 것에 대한 결과를 전달한다.
// 성공했을 때에는 인자로 넣은 iSize만큼 모두 넣거나 꺼내는 작업을 성공한 것이다.
bool RingBuffer::EnqueueRetBool(char* chpData, int iSize)
{
	if (GetFreeSize() < iSize) // 남아있는 용량 (== GetFreeSize)이 집어넣으려는 용량보다 작으면 실패 return;
	{
		return false;
	}

	int direct_enqueue_size = DirectEnqueueSize();

	if (direct_enqueue_size < iSize)
	{
		memcpy_s(&m_chpBuffer[m_iRear], direct_enqueue_size, chpData, direct_enqueue_size);
		MoveRear(direct_enqueue_size);

		int extra_enqueue_size = iSize - direct_enqueue_size;

		memcpy_s(&m_chpBuffer[m_iRear], extra_enqueue_size, chpData + direct_enqueue_size, extra_enqueue_size);
		MoveRear(extra_enqueue_size);
	}
	else
	{
		memcpy_s(&m_chpBuffer[m_iRear], iSize, chpData, iSize);
		MoveRear(iSize);
	}

	return true;
}

bool RingBuffer::DequeueRetBool(char* chpDest, int iSize) // 저장되어 있는 용량 (== GetUseSize)이 빼려는 용량보다 작으면 실패 return;
{
	if (GetUseSize() < iSize)
	{
		return false;
	}

	int direct_dequeue_size = DirectDequeueSize();

	if (iSize > direct_dequeue_size)
	{
		memcpy_s(chpDest, direct_dequeue_size, &m_chpBuffer[m_iFront], direct_dequeue_size);
		MoveFront(direct_dequeue_size);

		int extra_dequeue_size = iSize - direct_dequeue_size;
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, &m_chpBuffer[m_iFront], extra_dequeue_size);
		MoveFront(extra_dequeue_size);
	}
	else
	{
		memcpy_s(chpDest, iSize, &m_chpBuffer[m_iFront], iSize);
		MoveFront(iSize);
	}
	return true;
}

bool RingBuffer::PeekRetBool(char* chpDest, int iSize) // Deque와 마찬가지로 성공,실패 반환. 임시 프론트 활용.
{

	if (GetUseSize() < iSize)
	{
		return false;
	}

	int direct_dequeue_size = DirectDequeueSize();

	if (iSize > direct_dequeue_size)
	{
		memcpy_s(chpDest, direct_dequeue_size, &m_chpBuffer[m_iFront], direct_dequeue_size);
		int prev_front = m_iFront;

		int extra_dequeue_size = iSize - direct_dequeue_size;
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, &m_chpBuffer[m_iFront], extra_dequeue_size);

		m_iFront = prev_front;
	}
	else
	{
		memcpy_s(chpDest, iSize, &m_chpBuffer[m_iFront], iSize);
	}
	return true;
}

//===== 막 집어넣는 경우.
int RingBuffer::Enqueue(char* chpData, int iSize)
{
	int _freeSize = GetFreeSize();
	int enqueueSize = iSize;

	if (_freeSize < iSize)
	{
		enqueueSize = _freeSize;
	}

	int direct_enqueue_size = DirectEnqueueSize();

	if (direct_enqueue_size < enqueueSize)
	{
		memcpy_s(&m_chpBuffer[m_iRear], direct_enqueue_size, chpData, direct_enqueue_size);
		MoveRear(direct_enqueue_size);

		int extra_enqueue_size = enqueueSize - direct_enqueue_size;

		memcpy_s(&m_chpBuffer[m_iRear], extra_enqueue_size, chpData + direct_enqueue_size, extra_enqueue_size);
		MoveRear(extra_enqueue_size);
	}
	else
	{
		memcpy_s(&m_chpBuffer[m_iRear], enqueueSize, chpData, enqueueSize);
		MoveRear(enqueueSize);
	}

	//m_iSize += enqueueSize;
	return enqueueSize;
}
int RingBuffer::Dequeue(char* chpDest, int iSize)
{
	int use_size = GetUseSize();
	int dequeueSize = iSize;

	if (use_size < iSize)
		dequeueSize = use_size;

	int direct_dequeue_size = DirectDequeueSize();

	if (dequeueSize > direct_dequeue_size)
	{
		memcpy_s(chpDest, direct_dequeue_size, &m_chpBuffer[m_iFront], direct_dequeue_size);
		MoveFront(direct_dequeue_size);

		int extra_dequeue_size = dequeueSize - direct_dequeue_size;
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, &m_chpBuffer[m_iFront], extra_dequeue_size);
		MoveFront(extra_dequeue_size);
	}
	else
	{
		memcpy_s(chpDest, dequeueSize, &m_chpBuffer[m_iFront], dequeueSize);
		MoveFront(dequeueSize);
	}

	//m_iSize -= dequeueSize;
	return dequeueSize;
}
int RingBuffer::Peek(char* chpDest, int iSize)
{
	int peekSize = iSize;

	int useSize = GetUseSize();
	if (useSize < iSize)
	{
		peekSize = useSize;
	}

	int direct_dequeue_size = DirectDequeueSize();

	if (peekSize > direct_dequeue_size)
	{
		memcpy_s(chpDest, direct_dequeue_size, &m_chpBuffer[m_iFront], direct_dequeue_size);
		int prev_front = m_iFront;

		int extra_dequeue_size = peekSize - direct_dequeue_size;
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, &m_chpBuffer[m_iFront], extra_dequeue_size);

		m_iFront = prev_front;
	}
	else
	{
		memcpy_s(chpDest, peekSize, &m_chpBuffer[m_iFront], peekSize);
	}

	return peekSize;
}
int RingBuffer::MoveRear(int iSize)
{
	int _imoveSize = iSize;
	int free_size = GetFreeSize();
	if (free_size < iSize) // 남아있는 용량 (== GetFreeSize)이 이동시키려는 용량보다 작으면 실패 return;
	{
		_imoveSize = m_iCapacity - free_size;
	}

	m_iRear = (m_iRear + _imoveSize) % m_iCapacity;

	//m_iSize += _imoveSize;
	return _imoveSize;
}

int RingBuffer::MoveFront(int iSize)
{
	int _imoveSize = iSize;
	int use_size = GetUseSize();
	if (use_size < iSize)
	{
		_imoveSize = use_size;
	}

	m_iFront = (m_iFront + _imoveSize) % m_iCapacity;

	//m_iSize -= _imoveSize;
	return _imoveSize;
}

bool RingBuffer::MoveRearRetBool(int iSize)
{
	if (GetFreeSize() < iSize) // 남아있는 용량 (== GetFreeSize)이 이동시키려는 용량보다 작으면 실패 return;
	{
		return false;
	}

	m_iRear = (m_iRear + iSize) % m_iCapacity;

	return true;
}

bool RingBuffer::MoveFrontRetBool(int iSize)
{
	if (GetUseSize() < iSize)
	{
		return false;
	}

	m_iFront = (m_iFront + iSize) % m_iCapacity;

	return true;
}

void RingBuffer::ClearBuffer()
{
	m_iFront = m_iRear;


	//m_iSize = 0;
}

char* RingBuffer::GetFrontBufferPtr()
{
	return &m_chpBuffer[m_iFront];
}

char* RingBuffer::GetRearBufferPtr()
{
	return &m_chpBuffer[m_iRear];
}

char* RingBuffer::GetStartBufferPtr()
{
	return m_chpBuffer;
}
bool RingBuffer::IsEmpty()
{
	return m_iRear == m_iFront;
}

bool RingBuffer::IsFull()
{
	return m_iFront == (m_iRear + 1) % m_iCapacity;
}

