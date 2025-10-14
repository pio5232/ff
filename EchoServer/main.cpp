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
//			wprintf(L" [Process] MaxWorkingSet : [%0.4f MB]\n", (double)pmc.PeakWorkingSetSize / (1024 * 1024)); // 제일 많았을 때 기준
//			wprintf(L" [Process] MaxPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakPagedPoolUsage / (1024 * 1024)); // Paged-pool 사용량 최대치
//			wprintf(L" [Process] MaxNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakNonPagedPoolUsage / (1024 * 1024)); //Non Paged-pool 사용량 최대치
//			wprintf(L" [Process] maxPagefileUsage : [%0.4f MB]\n\n", (double)pmc.PeakPagefileUsage / (1024 * 1024)); // 가상 메모리 커밋된 크기 최대치
//
//			wprintf(L" [Process] CurrentWorkingSet : [%0.4f MB]\n", (double)pmc.WorkingSetSize / (1024 * 1024));  // 프로세스가 직접적으로 접근가능한, 메모리에 존재하는 프레임들
//			wprintf(L" [Process] CurrentPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPagedPoolUsage / (1024 * 1024)); // 현재 Paged-pool 사용량
//			wprintf(L" [Process] CurrentNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaNonPagedPoolUsage / (1024 * 1024)); // 현재 Non Paged-pool 사용량
//			wprintf(L" [Process] PageFileUsage : [%0.4f MB]\n", (double)pmc.PagefileUsage / (1024 * 1024)); // 현재 가상 메모리 커밋된 크기
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

#include "pch.h" // 프로젝트의 Precompiled Header
#include <iostream>
#include <vector>
#include <thread>
#include "Logger.h"
#include "Memory.h"

// 각 스레드가 실행할 로그 기록 함수
void LogWorker(int threadId, int logCount)
{
    for (int i = 0; i < logCount; ++i)
    {
        // 다양한 로그 레벨과 메시지로 로그를 기록합니다.
        _LOG(L"TestLog", LOG_LEVEL_INFO, L"Thread %d: Logging message %d.", threadId, i);
        _LOG(L"Performance", LOG_LEVEL_DEBUG, L"Thread %d: Performance test message %d.", threadId, i);

        if (i % 100 == 0) {
            _LOG(L"Important", LOG_LEVEL_SYSTEM, L"Thread %d: This is a system-level message, count: %d.", threadId, i);
        }
    }
}

int main()
{
    // 1. 전역 메모리 할당자와 로거를 초기화합니다.
    g_memAllocator = new jh_memory::MemoryAllocator();
    g_logger = new jh_utility::FileLogger();

    // 로그 레벨을 설정합니다. (DEBUG 이상의 모든 로그를 기록)
    SET_LOG_LEVEL(LOG_LEVEL_DEBUG);

    std::cout << "Logger test started with multiple threads..." << std::endl;

    // 2. 여러 스레드를 생성하여 동시에 로그를 기록하도록 합니다.
    const int numThreads = 10; // 테스트할 스레드 개수
    const int logsPerThread = 1000; // 스레드당 로그 개수

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(LogWorker, i, logsPerThread);
    }

    // 3. 모든 스레드가 작업을 마칠 때까지 기다립니다.
    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    std::cout << "All threads finished logging." << std::endl;
    std::cout << "Waiting for logger to write remaining logs..." << std::endl;

    // 4. 로거가 큐에 남은 모든 로그를 처리하고 종료될 수 있도록 잠시 대기합니다.
    //    (소멸자가 자동으로 처리하므로 필수적이지는 않지만, 확인을 위해 추가)
    Sleep(500);


    std::cout << "Logger test finished. Check the FileLog directory." << std::endl;

    return 0;
}