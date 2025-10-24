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

thread_local jh_utility::ThreadProfileData* tls_pProfiler = nullptr;

jh_utility::ProfileSample::ProfileSample()
{
	Initialize();
}

void jh_utility::ProfileSample::Initialize()
{
	memset(m_wszSampleName, NULL, sizeof(m_wszSampleName));
	m_llStartTime.QuadPart = 0;
	m_ullTotalTime = 0;
	m_ullMinTime[0] = m_ullMinTime[1] = ULLONG_MAX;
	m_ullMaxTime[0] = m_ullMaxTime[1] = 0;
	m_ullCallCount = 0;

	m_dwThreadId = ~(0);
	m_bUseFlag = false;
}

jh_utility::ThreadProfileData::ThreadProfileData() : m_samples{}
{
}


void jh_utility::ThreadProfileData::Start(const WCHAR* tag)
{
	for (int i = 0; i < MAX_SAMPLE_COUNT; i++)
	{
		if (false == m_samples[i].m_bUseFlag) // 첫 사용 체크
		{
			// 만약 스레드가 Stop()실행 중에 Reset -> 데이터 초기화돼서 찌꺼기가 남아있어도
			// 다시 시작할때 초기화해준다. 문제없지 않을까
			m_samples[i].m_bUseFlag = true;

			StringCchCopy(m_samples[i].m_wszSampleName, ARRAY_SIZE(m_samples[i].m_wszSampleName), tag);

			m_samples[i].m_ullTotalTime = 0;

			m_samples[i].m_ullMinTime[0] = m_samples[i].m_ullMinTime[1] = ULLONG_MAX;
			m_samples[i].m_ullMaxTime[0] = m_samples[i].m_ullMaxTime[1] = 0;
			
			m_samples[i].m_ullCallCount= 0;

			m_samples[i].m_dwThreadId = GetCurrentThreadId();

			QueryPerformanceCounter(&m_samples[i].m_llStartTime);

			break;
		}

		if (wcscmp(m_samples[i].m_wszSampleName, tag) == 0) // 동일한 태그인지 체크
		{
			QueryPerformanceCounter(&m_samples[i].m_llStartTime);

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
		if (false == m_samples[i].m_bUseFlag)
			break;

		if (wcscmp(m_samples[i].m_wszSampleName, tag) == 0)
		{
			// 초
			LONGLONG elapsedTime = (endTime.QuadPart - m_samples[i].m_llStartTime.QuadPart);

			if (elapsedTime < m_samples[i].m_ullMinTime[0]) // 뒤 1st 
			{
				m_samples[i].m_ullMinTime[1] = m_samples[i].m_ullMinTime[0];
				m_samples[i].m_ullMinTime[0] = elapsedTime;
			}
			else if (elapsedTime < m_samples[i].m_ullMinTime[1]) // 뒤 2st
			{
				m_samples[i].m_ullMinTime[1] = elapsedTime;
			}

			if (elapsedTime > m_samples[i].m_ullMaxTime[0]) // 앞 1st
			{
				m_samples[i].m_ullMaxTime[1] = m_samples[i].m_ullMaxTime[0];
				m_samples[i].m_ullMaxTime[0] = elapsedTime;
			}
			else if (elapsedTime > m_samples[i].m_ullMaxTime[1]) // 앞 2st
			{
				m_samples[i].m_ullMaxTime[1] = elapsedTime;
			}

			m_samples[i].m_ullTotalTime += elapsedTime;
			m_samples[i].m_ullCallCount++;

			break;
		}
	}
}

void jh_utility::ThreadProfileData::Reset()
{
	for (int i = 0; i < MAX_SAMPLE_COUNT; i++)
	{
		if (false == m_samples[i].m_bUseFlag)
			break;

		m_samples[i].Initialize();
	}
}

/// +-------------------+
///	|	ProfileManager	|
///	+-------------------+
/// 
void jh_utility::ProfileManager::Start(const WCHAR* funcName)
{
	if (nullptr == tls_pProfiler)
	{
		tls_pProfiler = new ThreadProfileData();

		DWORD currentThreadId = GetCurrentThreadId();
		// 스레드가 재사용된 경우 기존에 등록된 스레드ID의 동적할당 데이터 해제
		{
			SRWLockGuard lockGuard(&m_lock);

			if (m_profilerMap.end() == m_profilerMap.find(currentThreadId))
			{
				delete m_profilerMap[currentThreadId];
			}

			// 없었던 경우는 등록, 있었던 경우는 변경
			m_profilerMap[currentThreadId] = tls_pProfiler;
		}
	}

	tls_pProfiler->Start(funcName);
}

void jh_utility::ProfileManager::Stop(const WCHAR* funcName)
{
	if (nullptr == tls_pProfiler)
		CrashDump::Crash();

	tls_pProfiler->Stop(funcName);
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
	
	fwprintf_s(file, L"%15s | %70s | %17s | %17s | %17s | %15s |\n", L"THREADID", L"FUNC", L"AVERAGE", L"MIN",L"MAX", L"CALL");;
	fwprintf_s(file, L"-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\n");

	for (auto& pair : m_profilerMap)
	{
		for (int i = 0; i < ARRAY_SIZE(pair.second->m_samples); i++)
		{
			ProfileSample& sample = pair.second->m_samples[i];

			if (false == sample.m_bUseFlag)
				break;
		
			// min, max는 2번째꺼 출력.
			fwprintf_s(file, L"%15u | %70s | %15.6lf㎲ | %15.6lf㎲ | %15.6lf㎲ | %15u |\n", sample.m_dwThreadId, sample.m_wszSampleName,
				(double)sample.m_ullTotalTime / sample.m_ullCallCount * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // AVERAGE
				(double)sample.m_ullMinTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MIN
				(double)sample.m_ullMaxTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MAX
				sample.m_ullCallCount); // m_ullCallCount
		}
		fwprintf_s(file, L"\n");
	}

	fwprintf_s(file, L"-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

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
