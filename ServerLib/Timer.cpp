#include "LibraryPch.h"
#include "Timer.h"

jh_utility::Timer::Timer() : _start{}, _end{}
{
	QueryPerformanceFrequency(&_frequency);

}

void jh_utility::Timer::Start()
{
	QueryPerformanceCounter(&_start);
}

ULONGLONG jh_utility::GetTimeStamp()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

ULONGLONG jh_utility::GetTimeStampMicrosecond()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
