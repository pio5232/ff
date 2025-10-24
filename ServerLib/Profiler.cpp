#include "LibraryPch.h"
#include <sstream>
#include <iomanip>
#include <utility>
#include <string>
#include <strsafe.h>
// Working Set�� Current Set�� �и�����
// Working Set : Start -> End�� �� ���

// Current Set : A <-> B 
// Working Set�� Current Set���� ���

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
		if (false == m_samples[i].m_bUseFlag) // ù ��� üũ
		{
			// ���� �����尡 Stop()���� �߿� Reset -> ������ �ʱ�ȭ�ż� ��Ⱑ �����־
			// �ٽ� �����Ҷ� �ʱ�ȭ���ش�. �������� ������
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

		if (wcscmp(m_samples[i].m_wszSampleName, tag) == 0) // ������ �±����� üũ
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
			// ��
			LONGLONG elapsedTime = (endTime.QuadPart - m_samples[i].m_llStartTime.QuadPart);

			if (elapsedTime < m_samples[i].m_ullMinTime[0]) // �� 1st 
			{
				m_samples[i].m_ullMinTime[1] = m_samples[i].m_ullMinTime[0];
				m_samples[i].m_ullMinTime[0] = elapsedTime;
			}
			else if (elapsedTime < m_samples[i].m_ullMinTime[1]) // �� 2st
			{
				m_samples[i].m_ullMinTime[1] = elapsedTime;
			}

			if (elapsedTime > m_samples[i].m_ullMaxTime[0]) // �� 1st
			{
				m_samples[i].m_ullMaxTime[1] = m_samples[i].m_ullMaxTime[0];
				m_samples[i].m_ullMaxTime[0] = elapsedTime;
			}
			else if (elapsedTime > m_samples[i].m_ullMaxTime[1]) // �� 2st
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
		// �����尡 ����� ��� ������ ��ϵ� ������ID�� �����Ҵ� ������ ����
		{
			SRWLockGuard lockGuard(&m_lock);

			if (m_profilerMap.end() == m_profilerMap.find(currentThreadId))
			{
				delete m_profilerMap[currentThreadId];
			}

			// ������ ���� ���, �־��� ���� ����
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
		
			// min, max�� 2��°�� ���.
			fwprintf_s(file, L"%15u | %70s | %15.6lf�� | %15.6lf�� | %15.6lf�� | %15u |\n", sample.m_dwThreadId, sample.m_wszSampleName,
				(double)sample.m_ullTotalTime / sample.m_ullCallCount * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // AVERAGE
				(double)sample.m_ullMinTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MIN
				(double)sample.m_ullMaxTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MAX
				sample.m_ullCallCount); // m_ullCallCount
		}
		fwprintf_s(file, L"\n");
	}

	fwprintf_s(file, L"-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

	// �������� 
	// THREAD \t | NAME \t | ... �� ���·� �����ϸ� �� ��
	// StringSafe�Լ��� �̿��ؼ� ����ϵ��� �Ѵ�.


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
