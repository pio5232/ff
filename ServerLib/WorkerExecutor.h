#pragma once

namespace jh_utility
{
	class WorkerExecutor
	{
	public:
		WorkerExecutor(DWORD workerCount, const WCHAR* logName);
		~WorkerExecutor() {}

		void Execute(MainFunc func, LPVOID lparam);

		void Close();

	private:
		DWORD m_dwWorkerCount;
		HANDLE* m_hWorkerList;
		HANDLE m_hCloseEvent;
		const WCHAR* m_pcwszLogName;
	};
}

