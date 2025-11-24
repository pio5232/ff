#pragma once

namespace jh_content
{
	struct JobData
	{
		JobData(std::weak_ptr<JobQueue> owner, JobRef job) : m_owner{owner}, m_jobRef{ job } {}

		std::weak_ptr<JobQueue> m_owner;
		JobRef					m_jobRef;
	};
	struct TimerItem
	{
		bool operator<(const TimerItem& other) const
		{
			return m_ullExecuteTick > other.m_ullExecuteTick;
		}

		ULONGLONG m_ullExecuteTick{};
		JobData* m_pJobData{};
	};

	class JobTimer
	{
	public:

		JobTimer() :m_items{}, m_bDistributing{false} 
		{
			InitializeSRWLock(&m_itemLock);
		}
		// 어떤 대상이 어떤 작업을 하는지 예약
		void Reserve(ULONGLONG tickAfter, std::weak_ptr<JobQueue> owner, JobRef job);
		void Distribute(ULONGLONG now);
		void Clear();

	private:
		SRWLOCK m_itemLock;
		std::priority_queue <TimerItem, std::vector<TimerItem>,std::less<TimerItem>> m_items;
		std::atomic<bool> m_bDistributing;
	};
}
