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
		if (false == _samples[i].useFlag) // ù ��� üũ
		{
			// ���� �����尡 Stop()���� �߿� Reset -> ������ �ʱ�ȭ�ż� ��Ⱑ �����־
			// �ٽ� �����Ҷ� �ʱ�ȭ���ش�. �������� ������
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

		if (wcscmp(_samples[i].sampleName, tag) == 0) // ������ �±����� üũ
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
			// ��
			LONGLONG elapsedTime = (endTime.QuadPart - _samples[i].startTime.QuadPart);

			if (elapsedTime < _samples[i].minTime[0]) // �� 1st 
			{
				_samples[i].minTime[1] = _samples[i].minTime[0];
				_samples[i].minTime[0] = elapsedTime;
			}
			else if (elapsedTime < _samples[i].minTime[1]) // �� 2st
			{
				_samples[i].minTime[1] = elapsedTime;
			}

			if (elapsedTime > _samples[i].maxTime[0]) // �� 1st
			{
				_samples[i].maxTime[1] = _samples[i].maxTime[0];
				_samples[i].maxTime[0] = elapsedTime;
			}
			else if (elapsedTime > _samples[i].maxTime[1]) // �� 2st
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
		// �����尡 ����� ��� ������ ��ϵ� ������ID�� �����Ҵ� ������ ����
		{
			SRWLockGuard lockGuard(&m_lock);

			if (m_profilerMap.end() == m_profilerMap.find(currentThreadId))
			{
				delete m_profilerMap[currentThreadId];
			}

			// ������ ���� ���, �־��� ���� ����
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
		
			// min, max�� 2��°�� ���.
			fwprintf_s(file, L"%15u | %25s | %15.6lf�� | %15.6lf�� | %15.6lf�� | %15u |\n", sample.threadId, sample.sampleName,
				(double)sample.totalTime / sample.callCount * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // AVERAGE
				(double)sample.minTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MIN
				(double)sample.maxTime[1] * MICRO_SCALE(m_llQueryPerformanceFrequency.QuadPart), // MAX
				sample.callCount); // callCount
		}
		fwprintf_s(file, L"\n");
	}

	fwprintf_s(file, L"---------------------------------------------------------------------------------------------------------------------------\n");

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
