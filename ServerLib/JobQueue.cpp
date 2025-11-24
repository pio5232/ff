#include "LibraryPch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

void jh_content::JobQueue::Push(JobRef job, bool pushOnly)
{
	const int prevCount = m_jobCount.fetch_add(1);
	m_jobs.Push(job); 

	// 첫 번째로 Job을 푸쉬했을때
	if (0 == prevCount)
	{
		// 자신이 실행한다
		if (nullptr == g_tlsCurrentJobQueue && false == pushOnly)
		{
			Execute();
		}
		// 전역 큐에 넘긴다. (여유 있는 스레드가 실행하도록 분산)
		else
		{
			g_pGlobalQueue->Push(shared_from_this());
		}
	}
}

void jh_content::JobQueue::Execute()
{
	g_tlsCurrentJobQueue = this;
	
	while (1)
	{
		std::queue<JobRef> jobs;
		m_jobs.Swap(jobs);

		const int jobCount = static_cast<int>(jobs.size());

		while (jobs.size() > 0)
		{
			JobRef& jobRef = jobs.front();

			jobRef->Execute();

			jobs.pop();
		}

		// 모든 작업을 처리한 경우 
		if (m_jobCount.fetch_sub(jobCount) == jobCount)
		{
			g_tlsCurrentJobQueue = nullptr;
			return;
		}

		const ULONGLONG now = jh_utility::GetTimeStamp();
		
		// 작업이 남아있고, 일정 시간이 지난 상태
		if (now >= g_tlsEndTickCount)
		{
			g_tlsCurrentJobQueue = nullptr;

			// 전역 큐에 넘긴다. (여유 있는 스레드가 실행하도록 분산)
			g_pGlobalQueue->Push(shared_from_this());
			break;
		}
	}
}
