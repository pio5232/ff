#pragma once
#include <Windows.h>

#include <map>

#define MAX_SAMPLE_COUNT 50
#define SAMPLE_NAME_LEN 64

#define PROF_WIDE2(x) L##x
#define PROF_WIDE1(x) PROF_WIDE2(x)
#define PROF_WFUNC PROF_WIDE1(__FUNCTION__)

#define PRO_START_AUTO_FUNC jh_utility::AutoProfiling ra(PROF_WFUNC)

#define PRO_START(x) jh_utility::ProfileManager::GetInstance().Start(L##x)
#define PRO_END(x) jh_utility::ProfileManager::GetInstance().Stop(L##x)

//#define PRO_START(x) CProfileManager::GetInstance().Start(L##x)
//#define PRO_END(x) CProfileManager::GetInstance().Stop(L##x)

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

		WCHAR m_wszSampleName[SAMPLE_NAME_LEN]; // �±� �̸�

		LARGE_INTEGER m_llStartTime; // ���� �ð�

		ULONGLONG m_ullTotalTime; // ��ü ��� �ð�
		ULONGLONG m_ullMinTime[2]; // �ּ� ��� �ð�
		ULONGLONG m_ullMaxTime[2]; // �ִ� ��� �ð�.
		ULONGLONG m_ullCallCount; // ȣ�� Ƚ�� 

		DWORD m_dwThreadId;

		bool m_bUseFlag; // ���������� ��� ����
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
	
		ProfileSample m_samples[MAX_SAMPLE_COUNT];
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

	class AutoProfiling
	{
		public :
			AutoProfiling(const WCHAR* functionName) : currentFunctionName{ functionName } 
			{
				jh_utility::ProfileManager::GetInstance().Start(currentFunctionName);
			}
			~AutoProfiling() {
				
				jh_utility::ProfileManager::GetInstance().Stop(currentFunctionName);
				currentFunctionName = nullptr;
			}

	private:
		const WCHAR* currentFunctionName = nullptr;
	};
}
extern thread_local jh_utility::ThreadProfileData* tls_pProfiler;