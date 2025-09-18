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
			QueryPerformanceCounter(&_end);

			T ret = static_cast<T>(_end.QuadPart - _start.QuadPart) / _frequency.QuadPart;

			_start.QuadPart = 0;

			return ret;
		}

		// end - start
		// start = end
		template<typename T = double>
		T Lap()
		{
			QueryPerformanceCounter(&_end);

			T ret = static_cast<T>(_end.QuadPart - _start.QuadPart) / _frequency.QuadPart;

			_start = _end;

			return ret;
		}
	private:
		LARGE_INTEGER _start;
		LARGE_INTEGER _end;
		LARGE_INTEGER _frequency;

	};

	// 1970_01_01 이후 시간 ms로 얻어오기
	ULONGLONG GetTimeStamp();
	
	ULONGLONG GetTimeStampMicrosecond();
}

