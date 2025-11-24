#include "LibraryPch.h"
#include "JobTimer.h"

void jh_content::JobTimer::Reserve(uint64 tickAfter, std::weak_ptr<JobQueue> owner, JobRef job)
{
	const ULONGLONG executeTick = jh_utility::GetTimeStamp() + tickAfter;
	
	JobData* jobData = jh_memory::ObjectPool<JobData>::Alloc(owner, job);

	SRWLockGuard lockGuard(&m_itemLock);
	
	m_items.push(TimerItem{ executeTick, jobData });
}

void jh_content::JobTimer::Distribute(uint64 now)
{
	if (m_bDistributing.exchange(true) == true)
		return;

	std::vector<TimerItem> items;

	{
		SRWLockGuard lockGuard(&m_itemLock);

		while (m_items.size() > 0)
		{
			const TimerItem& timerItem = m_items.top();
			if (now < timerItem.m_ullExecuteTick)
				break;

			items.push_back(timerItem);
			m_items.pop();
		}
	}

	for (TimerItem& item : items)
	{
		if (JobQueueRef owner = item.m_pJobData->m_owner.lock())
			owner->Push(item.m_pJobData->m_jobRef);

		jh_memory::ObjectPool<JobData>::Free(item.m_pJobData);
	}

	m_bDistributing.store(false);
}

void jh_content::JobTimer::Clear()
{
}
