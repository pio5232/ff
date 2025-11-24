#include "LibraryPch.h"
#include "SerializationBuffer.h"
#include "ObjectPool.h"

using namespace jh_utility;

alignas(64) ULONGLONG SerializationBuffer::g_ullPacketCount = 0;

SerializationBuffer::SerializationBuffer(size_t iBufferSize) : m_iBufferCapacity(iBufferSize), m_iFront(0), m_iRear(0)
{
	m_chpBuffer = static_cast<char*>(g_pMemSystem->Alloc(m_iBufferCapacity));

	InterlockedIncrement64((LONGLONG*)&g_ullPacketCount);
}


SerializationBuffer::~SerializationBuffer()
{
	g_pMemSystem->Free(m_chpBuffer);

	m_chpBuffer = nullptr;

	InterlockedDecrement64((LONGLONG*)&g_ullPacketCount);
}

// 패킷 청소
void SerializationBuffer::Clear()
{
	m_iFront = 0;
	m_iRear = 0;
}
// 버퍼 포인터 얻기
char* SerializationBuffer::GetBufferPtr() const
{
	return m_chpBuffer;
}

// 버퍼 Pos 이동
int SerializationBuffer::MoveRearPos(int iSize)
{
	if (iSize <= 0)
		return 0;

	int freeSize = GetFreeSize();
	int moveSize;

	if (freeSize < iSize)
		moveSize = freeSize;
	else
		moveSize = iSize;

	m_iRear += moveSize;

	return moveSize;
}

int SerializationBuffer::MoveFrontPos(int iSize)
{
	if (iSize <= 0)
		return 0;

	int dataSize = GetDataSize();
	int moveSize = iSize;

	if (dataSize < iSize)
		moveSize = dataSize;

	m_iFront += moveSize;

	return moveSize;
}

int SerializationBuffer::GetData(char* chpDest, int iSize) // 바깥으로 데이터 빼기, Throw (int), 1 번 (뺄 사이즈가 요청한 사이즈보다 작음.) 
{
	int dataSize = GetDataSize();

	if (dataSize < iSize)
		return 0;

	errno_t errCpy = memcpy_s(chpDest, iSize, &m_chpBuffer[m_iFront], iSize);

	m_iFront += iSize;

	return iSize;
}
int SerializationBuffer::PutData(const char* chpSrc, int iSrcSize) // 데이터 넣기, Throw (int) , 0번 (넣을 사이즈 작음)
{
	if (GetFreeSize() < iSrcSize)
		return 0;

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), chpSrc, iSrcSize);

	m_iRear += iSrcSize;

	return iSrcSize;
}


// << 연산자 오버로딩, Throw (int) , 넣을 사이즈가 작을 때 0번 / 뺄 사이즈가 요청한 사이즈보다 작을 때 1번
SerializationBuffer& SerializationBuffer::operator<<(unsigned char ucValue)
{
	if (GetFreeSize() < sizeof(ucValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&ucValue, sizeof(ucValue));

	m_iRear += sizeof(ucValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(char cValue)
{
	if (GetFreeSize() < sizeof(cValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&cValue, sizeof(cValue));

	m_iRear += sizeof(cValue);

	return *this;
}

SerializationBuffer& jh_utility::SerializationBuffer::operator<<(bool bValue)
{
	if (GetFreeSize() < sizeof(bValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&bValue, sizeof(bValue));

	m_iRear += sizeof(bValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned short usValue)
{
	if (GetFreeSize() < sizeof(usValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&usValue, sizeof(usValue));

	m_iRear += sizeof(usValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(short shValue)
{
	if (GetFreeSize() < sizeof(shValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&shValue, sizeof(shValue));

	m_iRear += sizeof(shValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned int uiValue)
{
	if (GetFreeSize() < sizeof(uiValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&uiValue, sizeof(uiValue));

	m_iRear += sizeof(uiValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(int iValue)
{
	if (GetFreeSize() < sizeof(iValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&iValue, sizeof(iValue));

	m_iRear += sizeof(iValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned long ulValue)
{
	if (GetFreeSize() < sizeof(ulValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&ulValue, sizeof(ulValue));

	m_iRear += sizeof(ulValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(long lValue)
{
	if (GetFreeSize() < sizeof(lValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&lValue, sizeof(lValue));

	m_iRear += sizeof(lValue);

	return *this;
}

SerializationBuffer& jh_utility::SerializationBuffer::operator<<(long long llValue)
{
	if (GetFreeSize() < sizeof(llValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&llValue, sizeof(llValue));

	m_iRear += sizeof(llValue);

	return *this;
}

SerializationBuffer& jh_utility::SerializationBuffer::operator<<(unsigned long long ullValue)
{
	if (GetFreeSize() < sizeof(ullValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&ullValue, sizeof(ullValue));

	m_iRear += sizeof(ullValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(float fValue)
{
	if (GetFreeSize() < sizeof(fValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&fValue, sizeof(fValue));

	m_iRear += sizeof(fValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(double dValue)
{
	if (GetFreeSize() < sizeof(dValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&m_chpBuffer[m_iRear], GetFreeSize(), (void*)&dValue, sizeof(dValue));

	m_iRear += sizeof(dValue);

	return *this;
}



SerializationBuffer& SerializationBuffer::operator>>(unsigned char& ucValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(ucValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&ucValue, sizeof(ucValue), &m_chpBuffer[m_iFront], sizeof(ucValue));

	m_iFront += sizeof(ucValue);

	return *this;
}
SerializationBuffer& SerializationBuffer::operator>>(char& cValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(cValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&cValue, sizeof(cValue), &m_chpBuffer[m_iFront], sizeof(cValue));

	m_iFront += sizeof(cValue);

	return *this;
}

SerializationBuffer& jh_utility::SerializationBuffer::operator>>(bool& bValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(bValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&bValue, sizeof(bValue), &m_chpBuffer[m_iFront], sizeof(bValue));

	m_iFront += sizeof(bValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(unsigned short& usValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(usValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&usValue, sizeof(usValue), &m_chpBuffer[m_iFront], sizeof(usValue));

	m_iFront += sizeof(usValue);

	return *this;
}
SerializationBuffer& SerializationBuffer::operator>>(short& shValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(shValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&shValue, sizeof(shValue), &m_chpBuffer[m_iFront], sizeof(shValue));

	m_iFront += sizeof(shValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(unsigned int& uiValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(uiValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&uiValue, sizeof(uiValue), &m_chpBuffer[m_iFront], sizeof(uiValue));

	m_iFront += sizeof(uiValue);

	return *this;
}
SerializationBuffer& SerializationBuffer::operator>>(int& iValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(iValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&iValue, sizeof(iValue), &m_chpBuffer[m_iFront], sizeof(iValue));

	m_iFront += sizeof(iValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(unsigned long& ulValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(ulValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&ulValue, sizeof(ulValue), &m_chpBuffer[m_iFront], sizeof(ulValue));

	m_iFront += sizeof(ulValue);

	return *this;
}
SerializationBuffer& SerializationBuffer::operator>>(long& lValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(lValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&lValue, sizeof(lValue), &m_chpBuffer[m_iFront], sizeof(lValue));

	m_iFront += sizeof(lValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator>> (float& fValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(fValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&fValue, sizeof(fValue), &m_chpBuffer[m_iFront], sizeof(fValue));

	m_iFront += sizeof(fValue);

	return *this;
}

SerializationBuffer& SerializationBuffer::operator>> (double& dValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(dValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&dValue, sizeof(dValue), &m_chpBuffer[m_iFront], sizeof(dValue));

	m_iFront += sizeof(dValue);

	return *this;
}

SerializationBuffer& jh_utility::SerializationBuffer::operator>>(long long& llValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(llValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&llValue, sizeof(llValue), &m_chpBuffer[m_iFront], sizeof(llValue));

	m_iFront += sizeof(llValue);

	return *this;
}

SerializationBuffer& jh_utility::SerializationBuffer::operator>>(unsigned long long& ullValue)
{
	int dataSize = GetDataSize();

	if (dataSize < sizeof(ullValue))
		CrashDump::Crash();

	errno_t errCpy = memcpy_s(&ullValue, sizeof(ullValue), &m_chpBuffer[m_iFront], sizeof(ullValue));

	m_iFront += sizeof(ullValue);

	return *this;
}