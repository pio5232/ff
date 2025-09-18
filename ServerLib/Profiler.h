#pragma once
#include <Windows.h>

#include <map>

#define MAX_SAMPLE_COUNT 50
#define SAMPLE_NAME_LEN 64

#define PROF_WIDE2(x) L##x
#define PROF_WIDE1(x) PROF_WIDE2(x)
#define PROF_WFUNC PROF_WIDE1(__FUNCTION__)

#define PRO_START_AUTO_FUNC jh_utility::ProfileManager::GetInstance().Start(PROF_WFUNC)
#define PRO_END_AUTO_FUNC jh_utility::ProfileManager::GetInstance().Stop(PROF_WFUNC)

#define PRO_START(x) CProfileManager::GetInstance().Start(L##x)
#define PRO_END(x) CProfileManager::GetInstance().Stop(L##x)

#define PRO_RESET jh_utility::ProfileManager::GetInstance().DataReset()
#define PRO_SAVE(FILE_NAME) jh_utility::ProfileManager::GetInstance().ProfileDataOutText(L##FILE_NAME) 
#define ARRAY_SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

#define MILLI_SCALE(qpfrequency) ((double)1'000 / (double)qpfrequency)
#define MICRO_SCALE(qpfrequency) ((double)1'000'000 / (double)qpfrequency)

// ���� �����尡 ��� ������ ���� �Ŀ� ����Ǵ� ���¶�� ���� ���� ���̴�.
namespace jh_utility
{
	struct ProfileSample
	{
		ProfileSample();
		void Initialize();

		WCHAR sampleName[SAMPLE_NAME_LEN]; // �±� �̸�

		LARGE_INTEGER startTime; // ���� �ð�

		ULONGLONG totalTime; // ��ü ��� �ð�
		ULONGLONG minTime[2]; // �ּ� ��� �ð�
		ULONGLONG maxTime[2]; // �ִ� ��� �ð�.
		ULONGLONG callCount; // ȣ�� Ƚ�� 

		DWORD threadId;

		bool useFlag; // ���������� ��� ����
	};

	struct ThreadProfileData
	{
	public:
		ThreadProfileData();
		//~ThreadProfileData();

		// Sample ����� �ִ� 50��
		void Start(const WCHAR* tag);
		void Stop(const WCHAR* tag);

		void Reset();
	
		ProfileSample _samples[MAX_SAMPLE_COUNT];
	};

	class ProfileManager
	{
	public:

		static ProfileManager& GetInstance()
		{
			static ProfileManager instance;

			return instance;
		}

		void Start(const WCHAR* funcName);
		void Stop(const WCHAR* funcName);

		void ProfileDataOutText(const WCHAR* fileName);
		void DataReset();

	private:
		LARGE_INTEGER m_llQueryPerformanceFrequency;

		ProfileManager();
		~ProfileManager();
		
		ProfileManager(const ProfileManager&) = delete;
		ProfileManager& operator=(const ProfileManager&) = delete;

		SRWLOCK m_lock;

		// ��ü ���Ḧ ���� �����Ҵ� ������ ����
		std::map<DWORD, ThreadProfileData*> m_profilerMap;
	
		// ������ ����ȭ���� �ʰ� �� ������ �����͸� ����Ѵ�.
		// ������ ���� �� �ִ�.

	};
}
extern thread_local jh_utility::ThreadProfileData* tls_profiler;