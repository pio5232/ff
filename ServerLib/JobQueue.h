#pragma once

#include "JobTimer.h"

namespace jh_content
{
	class JobQueue : public std::enable_shared_from_this<JobQueue>
	{
	public:
		JobQueue() : m_jobs{}, m_jobCount{} {}

		// 작업을 자신의 작업 큐에 넣는다.
		void DoAsync(CallbackType&& callback)
		{
			Push(jh_memory::MakeShared<Job>(std::move(callback)));
		}

		template <typename T, typename Ret, typename... Args>
		void DoAsync(Ret(T::* memFunc), Args... args)
		{
			std::shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());

			Push(jh_memory::MakeShared<Job>(owner, memFunc, std::forward<Args>(args)...);
		}

		// 예약
		void DoTimer(ULONGLONG tickAfter, CallbackType&& callback)
		{
			JobRef job = jh_memory::MakeShared<Job>(std::move(callback));
			
			g_pJobTimer->Reserve(tickAfter, shared_from_this(), job);
		}

		template <typename T, typename Ret, typename... Args>
		void DoTimer(ULONGLONG tickAfter, Ret(T::* memFunc)(Args...), Args... args)
		{
			std::shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
			JobRef job = jh_memory::MakeShared<Job>(owner, memFunc, std::forward<Args>(args)...);
			g_jobTimer->Reserve(tickAfter, shared_from_this(), job);
		}
			
		void ClearJobs() { m_jobs.Clear(); }
	public:
		void Push(JobRef job, bool pushOnly = false);
		void Execute();

	protected:
		jh_utility::LockQueue<JobRef> m_jobs;
		std::atomic<int> m_jobCount;
	};
}

