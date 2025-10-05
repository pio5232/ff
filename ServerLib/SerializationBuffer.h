#pragma once

namespace jh_memory
{
	class MemoryAllocator;
}
namespace jh_utility
{
#define CSERIALIZATION_DEFAULT_SIZE 1024
#define CSERIALIZATION_MAX_SIZE 2048
	class SerializationBuffer
	{
	public:
		SerializationBuffer(jh_memory::MemoryAllocator* memoryAllocator, size_t iBufferSize);
		~SerializationBuffer();

		SerializationBuffer(const SerializationBuffer& other) = delete;
		SerializationBuffer& operator= (const SerializationBuffer& other) = delete;

		SerializationBuffer(SerializationBuffer&& other) noexcept;
		SerializationBuffer& operator=(SerializationBuffer&& other) noexcept;

		// 패킷 청소
		void Clear();

		// 리사이즈.
		[[deprecated("사용하지 않음.")]] bool Resize();

		//버퍼 포인터 얻기
		char* GetBufferPtr() const;

		// 버퍼 사이즈 얻기
		int GetBufferSize() const
		{
			return m_iBufferCapacity;
		}

		// 현재 사용중인 사이즈 얻기
		int GetDataSize() const
		{
			return m_iRear - m_iFront;
		}

		int GetFreeSize() const
		{
			return m_iBufferCapacity - m_iRear;
		}

		int GetData(char* chpDest, int iSize);
		int PutData(const char* chpSrc, int iSrcSize);

		// 연산자 오버로딩
		SerializationBuffer& operator<<(unsigned char ucValue);
		SerializationBuffer& operator<<(char cValue);
		SerializationBuffer& operator<<(bool bValue);

		SerializationBuffer& operator<<(unsigned short usValue);
		SerializationBuffer& operator<<(short shValue);

		SerializationBuffer& operator<<(unsigned int uiValue);
		SerializationBuffer& operator<<(int iValue);

		SerializationBuffer& operator<<(unsigned long ulValue);
		SerializationBuffer& operator<<(long lValue);

		SerializationBuffer& operator<< (long long llValue);
		SerializationBuffer& operator<< (unsigned long long ullValue);

		SerializationBuffer& operator<< (float fValue);

		//SerializationBuffer& operator<< (__int64 llValue);
		SerializationBuffer& operator<< (double dValue);

		SerializationBuffer& operator>>(unsigned char& ucValue);
		SerializationBuffer& operator>>(char& cValue);
		SerializationBuffer& operator>>(bool& bValue);

		SerializationBuffer& operator>>(unsigned short& usValue);
		SerializationBuffer& operator>>(short& shValue);

		SerializationBuffer& operator>>(unsigned int& uiValue);
		SerializationBuffer& operator>>(int& iValue);

		SerializationBuffer& operator>>(unsigned long& ulValue);
		SerializationBuffer& operator>>(long& lValue);
		
		SerializationBuffer& operator>> (float& fValue);

		//SerializationBuffer& operator>> (__int64& llValue);
		SerializationBuffer& operator>> (double& dValue);

		SerializationBuffer& operator>> (unsigned long long& ullValue);
		SerializationBuffer& operator>> (long long& llValue);

		// 버퍼 포인터 얻기
		char* GetRearPtr() const
		{
			return &m_chpBuffer[m_iRear];
		}

		char* GetFrontPtr() const
		{
			return &m_chpBuffer[m_iFront];
		}

		// 버퍼 Pos 이동
		int MoveRearPos(int iSize);
		int MoveFrontPos(int iSize);

	public:
		static ULONGLONG g_ullPacketCount;
		
	private:
		int m_iBufferCapacity;
		int m_iFront;
		int m_iRear;
		char* m_chpBuffer;

		jh_memory::MemoryAllocator* m_pMemoryAllocator;
	};
}

//#endif // !CSerialization