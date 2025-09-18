#pragma once
#include "CountingBlock.h"

namespace jh_utility
{
	template <typename T>
	class ClearRefPtr
	{
	public:
		struct RefData;
	private:

		struct RefData
		{
		public:
			RefData() : m_countingBlock{ 0 }, m_data{ }
			{

			}
			CountingBlock m_countingBlock;
			T m_data;

			void Clear()
			{
				// Data의 Clear함수 구현 필수
				m_data.Clear();
			}
		};
	public:
		void Reset()
		{
			DecreaseRefCnt();
		}
		static void SetPool(jh_utility::ClearPool<RefData>* pool)
		{
			m_pClearPool = pool;
		}

		
		static ClearRefPtr Make()
		{
			if (nullptr == m_pClearPool)
				CrashDump::Crash();
			
			//RefData* pRefData = m_pClearPool->Alloc(std::forward<Args>(args...));
			RefData* pRefData = m_pClearPool->Alloc();
			
			if (nullptr == pRefData)
				CrashDump::Crash();

			return ClearRefPtr(pRefData);
			//m_pRefData = m_pClearPool->Alloc(std::forward<Args>(args...));
		
			//return m_pRefData;
		}

		ClearRefPtr() = delete;

		ClearRefPtr(const ClearRefPtr& other)
		{
			m_pRefData = other.m_pRefData;

			IncreaseRefCnt();
		}

		ClearRefPtr& operator= (const ClearRefPtr& other)
		{
			m_pRefData = other.m_pRefData;

			IncreaseRefCnt();

			return *this;
		}
		
		// 
		ClearRefPtr(ClearRefPtr&& other) noexcept
		{
			m_pRefData = other.m_pRefData;

			other.m_pRefData = nullptr;
		}

		// other 삭제
		ClearRefPtr& operator= (ClearRefPtr&& other)
		{
			if (this != &other)
			{
				DecreaseRefCnt();

				m_pRefData = other.m_pRefData;

				other.m_pRefData = nullptr;
			}
			return *this;
		}

		~ClearRefPtr()
		{
			DecreaseRefCnt();
		}

		T& operator*()
		{
			return m_pRefData->m_data;
		}

		T* operator->()
		{
			return &m_pRefData->m_data;
		}

	private:
		explicit ClearRefPtr(RefData* refData) 
		{
			m_pRefData = refData;

			IncreaseRefCnt();
		}
		void IncreaseRefCnt()
		{
			if (nullptr == m_pRefData)
				return; // jh_utility::CrashDump::Crash();// return;

			InterlockedIncrement(&m_pRefData->m_countingBlock.m_lRefCnt);
		}

		void DecreaseRefCnt()
		{
			if (nullptr == m_pRefData)
				return; // jh_utility::CrashDump::Crash();// return;

			if (0 == InterlockedDecrement(&m_pRefData->m_countingBlock.m_lRefCnt))
			{
				m_pClearPool->Free(m_pRefData);
			}
			m_pRefData = nullptr;
		}
		RefData* m_pRefData;

		static jh_utility::ClearPool<RefData>* m_pClearPool;
	};

}