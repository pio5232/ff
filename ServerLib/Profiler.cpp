#include "LibraryPch.h"
#include <sstream>
#include <iomanip>
#include <utility>
#include <string>
#include <strsafe.h>
// Working Set과 Current Set을 분리하자
// Working Set : Start -> End할 때 등록

// Current Set : A <-> B 
// Working Set은 Current Set으로 등록

using namespace jh_utility;

thread_local jh_utility::ThreadProfileData* tls_profiler = nullptr;

jh_utility::ProfileSample::ProfileSample()
{
	Initialize();
}

void jh_utility::ProfileSample::Initialize()
{
	memset(sampleName, NULL, sizeof(sampleName));
	startTime.QuadPart = 0;
	totalTime = 0;
	minTime[0] = minTime[1] = MAXULONGLONG;
	maxTime[0] = maxTime[1] = 0;
	callCount = 0;

	threadId = ~(0);
	useFlag = false;
}

jh_utility::ThreadProfileData::ThreadProfileData() : _samples{}
{
}


void jh_utility::ThreadProfileData::Start(const WCHAR* tag)
{
	for (int i = 0; i < MAX_SAMPLE_COUNT; i++)
	{
		if (false == _samples[i].useFlag) // 첫 사용 체크
		{
			// 만약 스레드가 Stop()실행 중에 Reset -> 데이터 초기화돼서 찌꺼기가 남아있어도
			// 다시 시작할때 초기화해준다. 문제없지 않을까
			_samples[i].useFlag = true;

			StringCchCopy(_samples[i].sampleName, ARRAY_SIZE(_samples[i].sampleName), tag);

			_samples[i].totalTime = 0;

			_samples[i].minTime[0] = _samples[i].minTime[1] = MAXULONGLONG;
			_samples[i].maxTime[0] = _samples[i].maxTime[1] = 0;
			
			_samples[i].callCount= 0;

			_samples[i].threadId = GetCurrentThreadId();

			QueryPerformanceCounter(&_samples[i].startTime);

			break;
		}

		if (wcscmp(_samples[i].sampleName, tag) == 0) // 동일한 태그인지 체크
		{
			QueryPerformanceCounter(&_samples[i].startTime);

			break;
		}

	}
}

void jh_utility::ThreadProfileData::Stop(const WCHAR* tag)
{
	LARGE_INTEGER endTime;

	QueryPerformanceCounter(&endTime);

	for (int i = 0; i < MAX_SAMPLE_COUNT; i++)
	{
		if (false == _samples[i].useFlag)
			break;

		if (wcscmp(_samples[i].sampleName, tag) == 0)
		{
			// 초
			LONGLONG elapsedTime = (endTime.QuadPart - _samples[i].startTime.QuadPart);

			if (elapsedTime < _samples[i].minTime[0]) // 뒤 1st 
			{
				_samples[i].minTime[1] = _samples[i].minTime[0];
				_samples[i].minTime[0] = elapsedTime;
			}
			else if (elapsedTime < _samples[i].minTime[1]) // 뒤 2st
			{
				_samples[i].minTime[1] = elapsedTime;
			}

			if (elapsedTime > _samples[i].maxTime[0]) // 앞 1st
			{
				_samples[i].maxTime[1] = _samples[i].maxTime[0];
				_samples[i].maxTime[0] = elapsedTime;
			}
			else if (elapsedTime > _samples[i].maxTime[1]) // 앞 2st
			{
				_samples[i].maxTime[1] = elapsedTime;
			}

			_samples[i].totalTime += elapsedTime;
			_samples[i].callCount++;

			break;
		}
	}
}

void jh_utility::ThreadProfileData::Reset()
{
	for (int i = 0; i < MAX_SAMPLE_COUNT; i++)
	{
		if (false == _samples[i].useFlag)
			break;

		_samples[i].Initialize();
	}
}

/// +-------------------+
///	|	ProfileManager	|
///	+-------------------+
/// 
void jh_utility::ProfileManager::Start(const WCHAR* funcName)
{
	if (nullptr == tls_profiler)
	{
		tls_profiler = new ThreadProfileData();

		DWORD currentThreadId = GetCurrentThreadId();
		// 스레드가 재사용된 경우 기존에 등록된 스레드ID의 동적할당 데이터 해제
		{
			SRWLockGuard lockGuard(&m_lock);

			if (m_profilerMap.end() == m_profilerMap.find(currentThreadId))
			{
				delete m_profilerMap[currentThreadId];
			}

			// 없었던 경우는 등록, 있었던 경우는 변경
			m_profilerMap[currentThreadId] = tls_profiler;
		}
	}

	tls_profiler->Start(funcName);
}

void jh_utility::ProfileManager::Stop(const WCHAR* funcName)
{
	if (nullptr == tls_profiler)
		CrashDump::Crash();

	tls_profiler->Stop(funcName);
}

jh_utility::ProfileManager::ProfileManager()
{
	InitializeSRWLock(&m_lock);

	_wsetlocale(LC_ALL, L"korean");

	QueryPerformanceFrequency(&m_llQueryPerformanceFrequency);
}
jh_utility::ProfileManager::~ProfileManager()
{
	for (auto& pair : m_profilerMap)
	{
		delete pair.second;
	}

	m_profilerMap.clear();

}

void jh_utility::ProfileManager::ProfileDataOutText(const WCHAR* fileName)
{
	// THREAD NAME AVERAGE MIN MAX CALL
	SRWLockGuard lockGuard(&m_lock);

	FILE* file = nullptr;

	errno_t wfOpenError =_wfopen_s(&file, fileName, L"w, ccs=UNICODE");

	if (0 != wfOpenError || nullptr == file)
	{
		_LOG(L"TLSProfiler", LOG_LEVEL_WARNING, L"ProfileDataOutText wfOpenError");
		
		return;
	}
	
	fwprintf_s(file, L"%15s | %25s | %17s | %17s | %17s | %15s |\n", L"THREADID", L"FUNC", L"AVERAGE", L"MIN",L"MAX", L"CALL");;
	fwprintf_s(file, L"---------------------------------------------------------------------------------------------------------------------------\n\n");

	for (auto& pair : m_profilerMap)
	{
		for (int i = 0; i < ARRAY_SIZE(pair.second->_samples); i++)
		{
			ProfileSample& sample = pair.second->_samples[i];

			if (false == sample.useFlag)
				break;
		
			// min, max는 2번째꺼 출력.
			fwprintf_s(file, L"%15u | %25s | %15.6lf㎲ | %15.6lf㎲ | %15.6lf㎲ | %15u |\n", sample.threadId, sample.sampleName,
				(double)sample.totalTime / sample.callCount * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // AVERAGE
				(double)sample.minTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MIN
				(double)sample.maxTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MAX
				sample.callCount); // callCount
		}
		fwprintf_s(file, L"\n");
	}

	fwprintf_s(file, L"---------------------------------------------------------------------------------------------------------------------------\n");

	// 우측정렬 
	// THREAD \t | NAME \t | ... 의 형태로 생성하면 될 듯
	// StringSafe함수를 이용해서 기록하도록 한다.


	fclose(file);
	return;
}

void jh_utility::ProfileManager::DataReset()
{
	SRWLockGuard lockGuard(&m_lock);

	for (auto& pair : m_profilerMap)
	{
		pair.second->Reset();
	}
}
