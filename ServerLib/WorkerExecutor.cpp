#include "LibraryPch.h"
#include "WorkerExecutor.h"
#include "ThreadExecutor.h"

jh_utility::WorkerExecutor::WorkerExecutor(DWORD workerCount, const WCHAR* logName) : m_dwWorkerCount{ workerCount }, m_pcwszLogName{logName}
{
	m_hCloseEvent = CreateEvent(nullptr, true, false, nullptr);
	
	if (nullptr == m_hCloseEvent)
		jh_utility::CrashDump::Crash();

	m_hWorkerList = static_cast<HANDLE*>(g_pMemSystem->Alloc(sizeof(HANDLE) * workerCount));

	_LOG(m_pcwszLogName, LOG_LEVEL_INFO, L"[WorkerExecutor] WorkerExecutor Count : [%u]", m_dwWorkerCount);

}

void jh_utility::WorkerExecutor::Execute(MainFunc func, LPVOID lparam)
{
	for (DWORD idx = 0; idx < m_dwWorkerCount; idx++)
	{
		m_hWorkerList[idx] = jh_utility::ThreadExecutor::Execute(func, lparam);
	}

	_LOG(m_pcwszLogName, LOG_LEVEL_INFO, L"[Execute] Execute ");
}

void jh_utility::WorkerExecutor::Close()
{
	SetEvent(m_hCloseEvent);

	DWORD ret = WaitForMultipleObjects(m_dwWorkerCount, m_hWorkerList, true, INFINITE);

	if (ret < WAIT_OBJECT_0 || ret >(WAIT_OBJECT_0 + m_dwWorkerCount - 1))
	{
		DWORD gle = GetLastError();
		_LOG(m_pcwszLogName, LOG_LEVEL_SYSTEM, L"[Close] WorkerExecutor WaitForMultipleObjects Failed. GetLastError : [%u]", gle);
	}

	g_pMemSystem->Free(m_hWorkerList);
	m_hWorkerList = nullptr;

	_LOG(m_pcwszLogName, LOG_LEVEL_INFO, L"[Close] Close ", );

}
