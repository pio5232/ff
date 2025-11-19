#pragma once

namespace jh_utility
{
	class Timer
	{
	public:
		Timer();
		void Start();

		// end - start 
		template<typename T = double>
		T Stop()
		{
			QueryPerformanceCounter(&m_end);

			T ret = static_cast<T>(m_end.QuadPart - m_start.QuadPart) / m_frequency.QuadPart;

			m_start.QuadPart = 0;

			return ret;
		}

		// end - start
		// start = end
		template<typename T = double>
		T Lap()
		{
			QueryPerformanceCounter(&m_end);

			T ret = static_cast<T>(m_end.QuadPart - m_start.QuadPart) / m_frequency.QuadPart;

			m_start = m_end;

			return ret;
		}
	private:
		LARGE_INTEGER m_start;
		LARGE_INTEGER m_end;
		LARGE_INTEGER m_frequency;

	};

	// 1970_01_01 이후 시간 ms로 얻어오기
	ULONGLONG GetTimeStamp();
	ULONGLONG GetTimeStampMicrosecond();
}

