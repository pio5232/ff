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

// resize�� ������ ū ������θ�.
bool RingBuffer::Resize(int newCapacity)
{
	if (newCapacity < m_iCapacity)
		return false;

	char* tmpBuffer = new char[newCapacity];

	int bufferSize = GetUseSize();
	bool ret = Dequeue(tmpBuffer, bufferSize);

	delete m_chpBuffer;

	// ���ο� ���ۿ� ���� ������ �籸��.
	m_chpBuffer = tmpBuffer;

	m_iCapacity = newCapacity;
	m_iFront = 0;
	m_iRear = bufferSize;

	return ret;
}

// Capcity return
int RingBuffer::GetCapacity()
{
	return m_iCapacity;
}

int RingBuffer::GetUseSize()
{
	int front = m_iFront;
	int rear = m_iRear;

	if (front == (rear + 1) % m_iCapacity)
		return m_iCapacity - 1;

	if (rear >= front)
		return rear - front;

	else
		return m_iCapacity - (front - rear);
}

int RingBuffer::GetFreeSize()
{
	int front = m_iFront;
	int rear = m_iRear;

	// �̰͸� ������
	if (front == (rear + 1) % m_iCapacity)
		return 0;

	else if (rear >= front)
		return m_iCapacity - (rear - front) - 1;
	else
		return front - rear - 1;
}

// �ᱹ empty <-> full�� m_rear == m_front ����
int RingBuffer::DirectEnqueueSize() // 
{
	int front = m_iFront;
	int rear = m_iRear;

	if (front == (rear + 1) % m_iCapacity)
		return 0;

	if (rear >= front)
	{
		if (front == 0)
			return m_iCapacity - rear - 1;

		return m_iCapacity - rear;
	}
	else
		return front - rear - 1;
}

int RingBuffer::DirectDequeueSize()
{
	int front = m_iFront;
	int rear = m_iRear;

	if (rear >= front)
		return rear - front;

	return m_iCapacity - front;

}
// =============================================

// �� ��å -> �����ۿ� �� �� �ִ� ũ�Ⱑ �ƴϸ� �ְų� ������ ���ϵ��� �Ѵ�.
// ���� ���� ���� �Ǵ� ���з� ������ ����� �ְų� ������ �Ϳ� ���� ����� �����Ѵ�.
// �������� ������ ���ڷ� ���� iSize��ŭ ��� �ְų� ������ �۾��� ������ ���̴�.
bool RingBuffer::EnqueueRetBool(char* chpData, int iSize)
{
	if (GetFreeSize() < iSize) // �����ִ� �뷮 (== GetFreeSize)�� ����������� �뷮���� ������ ���� return;
	{
		return false;
	}

	int direct_enqueue_size = DirectEnqueueSize();

	if (direct_enqueue_size < iSize)
	{
		memcpy_s(&m_chpBuffer[m_iRear], direct_enqueue_size, chpData, direct_enqueue_size);
		MoveRear(direct_enqueue_size);

		int extra_enqueue_size = iSize - direct_enqueue_size;

		memcpy_s(m_chpBuffer, extra_enqueue_size, chpData + direct_enqueue_size, extra_enqueue_size);
		MoveRear(extra_enqueue_size);
	}
	else
	{
		memcpy_s(&m_chpBuffer[m_iRear], iSize, chpData, iSize);
		MoveRear(iSize);
	}

	return true;
}

bool RingBuffer::DequeueRetBool(char* chpDest, int iSize) // ����Ǿ� �ִ� �뷮 (== GetUseSize)�� ������ �뷮���� ������ ���� return;
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
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, m_chpBuffer, extra_dequeue_size);
		MoveFront(extra_dequeue_size);
	}
	else
	{
		memcpy_s(chpDest, iSize, &m_chpBuffer[m_iFront], iSize);
		MoveFront(iSize);
	}
	return true;
}

bool RingBuffer::PeekRetBool(char* chpDest, int iSize) // Deque�� ���������� ����,���� ��ȯ. �ӽ� ����Ʈ Ȱ��.
{
	if (GetUseSize() < iSize)
	{
		return false;
	}

	int direct_dequeue_size = DirectDequeueSize();

	if (iSize > direct_dequeue_size)
	{
		memcpy_s(chpDest, direct_dequeue_size, &m_chpBuffer[m_iFront], direct_dequeue_size);

		int extra_dequeue_size = iSize - direct_dequeue_size;
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, m_chpBuffer, extra_dequeue_size);
	}
	else
	{
		memcpy_s(chpDest, iSize, &m_chpBuffer[m_iFront], iSize);
	}
	return true;
}

//===== �� ����ִ� ���.
int RingBuffer::Enqueue(char* chpData, int iSize)
{
	int freeSize = GetFreeSize();
	int enqueueSize = iSize;

	if (freeSize < iSize)
	{
		enqueueSize = freeSize;
	}

	int direct_enqueue_size = DirectEnqueueSize();

	if (direct_enqueue_size < enqueueSize)
	{
		memcpy_s(&m_chpBuffer[m_iRear], direct_enqueue_size, chpData, direct_enqueue_size);
		MoveRear(direct_enqueue_size);

		int extra_enqueue_size = enqueueSize - direct_enqueue_size;

		memcpy_s(m_chpBuffer, extra_enqueue_size, chpData + direct_enqueue_size, extra_enqueue_size);
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
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, m_chpBuffer, extra_dequeue_size);
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
		//MoveFrontRetBool(direct_dequeue_size);

		int extra_dequeue_size = peekSize - direct_dequeue_size;
		memcpy_s(chpDest + direct_dequeue_size, extra_dequeue_size, m_chpBuffer, extra_dequeue_size);
	}
	else
	{
		memcpy_s(chpDest, peekSize, &m_chpBuffer[m_iFront], peekSize);
	}

	return peekSize;
}


bool RingBuffer::MoveRear(int iSize)
{
	if (GetFreeSize() < iSize) // �����ִ� �뷮 (== GetFreeSize)�� �̵���Ű���� �뷮���� ������ ���� return;
	{
		return false;
	}

	m_iRear = (m_iRear + iSize) % m_iCapacity;

	//m_iSize += iSize;
	return true;
}

bool RingBuffer::MoveFront(int iSize)
{
	if (GetUseSize() < iSize)
	{
		return false;
	}

	m_iFront = (m_iFront + iSize) % m_iCapacity;
	 
	//m_iSize -= iSize;
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

