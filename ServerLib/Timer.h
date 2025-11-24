#pragma once

namespace jh_utility
{
	class Timer
	{
	public:
		Timer() : m_start{}, m_end{} {};
		void Start() { QueryPerformanceCounter(&m_start); }

		// end - start 
		template<typename T = double>
		T Stop()
		{
			QueryPerformanceCounter(&m_end);

			T ret = static_cast<T>(m_end.QuadPart - m_start.QuadPart) / frequency.QuadPart;

			m_start.QuadPart = 0;

			return ret;
		}

		// end - start
		// start = end
		template<typename T = double>
		T Lap()
		{
			QueryPerformanceCounter(&m_end);

			T ret = static_cast<T>(m_end.QuadPart - m_start.QuadPart) / frequency.QuadPart;

			m_start = m_end;

			return ret;
		}
		static ULONGLONG GetFrequency() { return frequency.QuadPart; }
	private:
		LARGE_INTEGER m_start;
		LARGE_INTEGER m_end;
		const inline static LARGE_INTEGER frequency = []() 
			{
				LARGE_INTEGER fq; 
				QueryPerformanceFrequency(&fq); 
				return fq; 
			}();

	};

	// ms´Â *1000
	// micro´Â *1000 *1000
	ULONGLONG jh_utility::GetTimeStamp()
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);

		return (t.QuadPart * 1'000) / jh_utility::Timer::GetFrequency();
	}

	ULONGLONG jh_utility::GetTimeStampMicrosecond()
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);

		return (t.QuadPart * 1'000'000) / jh_utility::Timer::GetFrequency();
	}
}

