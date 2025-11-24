#pragma once

namespace jh_content
{
	class GlobalQueue
	{
	public:
		GlobalQueue() {};
		~GlobalQueue() {};

		void			Push(JobQueueRef jobQueue) { m_jobQueues.Push(jobQueue); }
		JobQueueRef		Pop() { return m_jobQueues.Pop(); };

	private:
		jh_utility::LockQueue<JobQueueRef> m_jobQueues;
	};
}

