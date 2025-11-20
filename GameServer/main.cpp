
#include "pch.h"
#include <iostream>
#include <thread>
#include <conio.h>
#include "NetworkBase.h"
#include <crtdbg.h>
#include "GameServer.h"
#include "GamePlayer.h"
#include "User.h"
#include "Psapi.h"
#define SERVERPORT 8768

#pragma comment (lib, "Winmm.lib")

int main()
{
	timeBeginPeriod(1);
	SET_LOG_LEVEL(LOG_LEVEL_INFO);

	jh_content::GameServer gameServer;

	HANDLE processHandle = GetCurrentProcess();

	PROCESS_MEMORY_COUNTERS pmc;

	if (false == gameServer.Start())
	{
		_LOG(L"Main", LOG_LEVEL_WARNING, L"Server Begin != ErrorCode::NONE");
		return 0;
	}
	while (1)
	{
		if (_kbhit())
		{
			char c = _getch();

			if (c == 'Q' || c == 'q')
				break;
		}
		if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
		{
			wprintf(L"=================================================\n");
			wprintf(L"                PROCESS MONITORING\n");
			wprintf(L"=================================================\n");
			wprintf(L" [Process] MaxWorkingSet : [%0.4f MB]\n", (double)pmc.PeakWorkingSetSize / (1024 * 1024)); // 제일 많았을 때 기준
			wprintf(L" [Process] MaxPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakPagedPoolUsage / (1024 * 1024)); // Paged-pool 사용량 최대치
			wprintf(L" [Process] MaxNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakNonPagedPoolUsage / (1024 * 1024)); //Non Paged-pool 사용량 최대치
			wprintf(L" [Process] maxPagefileUsage : [%0.4f MB]\n\n", (double)pmc.PeakPagefileUsage / (1024 * 1024)); // 가상 메모리 커밋된 크기 최대치

			wprintf(L" [Process] CurrentWorkingSet : [%0.4f MB]\n", (double)pmc.WorkingSetSize / (1024 * 1024));  // 프로세스가 직접적으로 접근가능한, 메모리에 존재하는 프레임들
			wprintf(L" [Process] CurrentPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPagedPoolUsage / (1024 * 1024)); // 현재 Paged-pool 사용량
			wprintf(L" [Process] CurrentNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaNonPagedPoolUsage / (1024 * 1024)); // 현재 Non Paged-pool 사용량
			wprintf(L" [Process] PageFileUsage : [%0.4f MB]\n", (double)pmc.PagefileUsage / (1024 * 1024)); // 현재 가상 메모리 커밋된 크기
		}

		wprintf(L"=================================================\n");
		wprintf(L"               SERVER MONITORING\n");
		wprintf(L"=================================================\n");
		wprintf(L" Press 'q' to shut down\n");
		wprintf(L" Port : %hu", gameServer.GetPort());
		wprintf(L"-------------------------------------------------\n");

		gameServer.Monitor();
		wprintf(L"-------------------------------------------------\n");
		wprintf(L" [Content] Total Users : %llu\n", jh_content::GamePlayer::GetAliveGamePlayerCount());
		wprintf(L" [Content] Total Rooms : %d\n", jh_content::User::GetAliveGameUserCount());
		wprintf(L"=================================================\n\n\n");

		Sleep(1000);
	}

	gameServer.Stop();
}