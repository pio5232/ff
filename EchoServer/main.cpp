//#include "pch.h"
//#include <Windows.h>
//#include <iostream>
//#include <conio.h>
//#include <Psapi.h>
//#include <crtdbg.h>
//#include "EchoServer.h"
//
//// C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.38.33130\include
//int main()
//{
//	SET_LOG_LEVEL(LOG_LEVEL_INFO);
//
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//
//	jh_content::EchoServer echoServer;
//
//	if (true != echoServer.Start())
//	{
//		printf("EchoServer Begin() Return != NONE");
//		return 0;
//	}
//
//	echoServer.Listen();
//
//	HANDLE processHandle = GetCurrentProcess();
//
//	PROCESS_MEMORY_COUNTERS pmc;
//
//	while (1)
//	{
//		if (_kbhit())
//		{
//			char c = _getch();
//
//			if (c == 'Q' || c == 'q')
//				break;
//		}
//
//		system("cls");
//		if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
//		{
//			wprintf(L"=================================================\n");
//			wprintf(L"                PROCESS MONITORING\n");
//			wprintf(L"=================================================\n");
//			wprintf(L" [Process] MaxWorkingSet : [%0.4f MB]\n", (double)pmc.PeakWorkingSetSize / (1024 * 1024)); // ���� ������ �� ����
//			wprintf(L" [Process] MaxPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakPagedPoolUsage / (1024 * 1024)); // Paged-pool ��뷮 �ִ�ġ
//			wprintf(L" [Process] MaxNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakNonPagedPoolUsage / (1024 * 1024)); //Non Paged-pool ��뷮 �ִ�ġ
//			wprintf(L" [Process] maxPagefileUsage : [%0.4f MB]\n\n", (double)pmc.PeakPagefileUsage / (1024 * 1024)); // ���� �޸� Ŀ�Ե� ũ�� �ִ�ġ
//
//			wprintf(L" [Process] CurrentWorkingSet : [%0.4f MB]\n", (double)pmc.WorkingSetSize / (1024 * 1024));  // ���μ����� ���������� ���ٰ�����, �޸𸮿� �����ϴ� �����ӵ�
//			wprintf(L" [Process] CurrentPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPagedPoolUsage / (1024 * 1024)); // ���� Paged-pool ��뷮
//			wprintf(L" [Process] CurrentNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaNonPagedPoolUsage / (1024 * 1024)); // ���� Non Paged-pool ��뷮
//			wprintf(L" [Process] PageFileUsage : [%0.4f MB]\n", (double)pmc.PagefileUsage / (1024 * 1024)); // ���� ���� �޸� Ŀ�Ե� ũ��
//		}
//		wprintf(L"=================================================\n");
//		wprintf(L"               SERVER MONITORING\n");
//		wprintf(L"=================================================\n");
//		wprintf(L" Press 'q' to shut down\n");
//		wprintf(L"-------------------------------------------------\n");
//
//		echoServer.Monitor();
//		wprintf(L"=================================================\n");
//
//
//		Sleep(1000);
//		//GetProcessTimes();
//		//GetProcessMemoryInfo();
//	}
//
//	echoServer.Stop();
//
//}

#include "pch.h" // ������Ʈ�� Precompiled Header
#include <iostream>
#include <vector>
#include <thread>
#include "Logger.h"
#include "Memory.h"

// �� �����尡 ������ �α� ��� �Լ�
void LogWorker(int threadId, int logCount)
{
    for (int i = 0; i < logCount; ++i)
    {
        // �پ��� �α� ������ �޽����� �α׸� ����մϴ�.
        _LOG(L"TestLog", LOG_LEVEL_INFO, L"Thread %d: Logging message %d.", threadId, i);
        _LOG(L"Performance", LOG_LEVEL_DEBUG, L"Thread %d: Performance test message %d.", threadId, i);

        if (i % 100 == 0) {
            _LOG(L"Important", LOG_LEVEL_SYSTEM, L"Thread %d: This is a system-level message, count: %d.", threadId, i);
        }
    }
}

int main()
{
    // 1. ���� �޸� �Ҵ��ڿ� �ΰŸ� �ʱ�ȭ�մϴ�.
    g_memAllocator = new jh_memory::MemoryAllocator();
    g_logger = new jh_utility::FileLogger();

    // �α� ������ �����մϴ�. (DEBUG �̻��� ��� �α׸� ���)
    SET_LOG_LEVEL(LOG_LEVEL_DEBUG);

    std::cout << "Logger test started with multiple threads..." << std::endl;

    // 2. ���� �����带 �����Ͽ� ���ÿ� �α׸� ����ϵ��� �մϴ�.
    const int numThreads = 10; // �׽�Ʈ�� ������ ����
    const int logsPerThread = 1000; // ������� �α� ����

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(LogWorker, i, logsPerThread);
    }

    // 3. ��� �����尡 �۾��� ��ĥ ������ ��ٸ��ϴ�.
    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    std::cout << "All threads finished logging." << std::endl;
    std::cout << "Waiting for logger to write remaining logs..." << std::endl;

    // 4. �ΰŰ� ť�� ���� ��� �α׸� ó���ϰ� ����� �� �ֵ��� ��� ����մϴ�.
    //    (�Ҹ��ڰ� �ڵ����� ó���ϹǷ� �ʼ��������� ������, Ȯ���� ���� �߰�)
    Sleep(500);


    std::cout << "Logger test finished. Check the FileLog directory." << std::endl;

    return 0;
}