#include "pch.h"
#include "Logger.h"
#include "LobbyServer.h"
#include "LobbyDefine.h"
#include "LanServer.h"
#include <conio.h>
#include <Psapi.h>

#include "User.h"
#include "Room.h"

#ifdef _DEBUG
	#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


using namespace jh_network;


int main()
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//
	//int* sf = new int;

	//_CrtDumpMemoryLeaks();

	//return 0; 
	SET_LOG_LEVEL(LOG_LEVEL_INFO);

	jh_content::LobbyServer lobbyServer;

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (false == lobbyServer.Start())
	{
		_LOG(L"Main", LOG_LEVEL_WARNING, L"Server Begin != ErrorCode::NONE");
		return 0;
	}

	//while (1)
	//{
	//	if (_kbhit())
	//	{
	//		char c = _getch();
	//		if (c == 'q' || c == 'Q')
	//			break;
	//	}
	//}

	//HANDLE processHandle = GetCurrentProcess();
	jh_utility::ProcessTimeInfo processTimeInfo{};
	SYSTEM_INFO sysInfo;

	HANDLE processHandle = GetCurrentProcess();

	PROCESS_MEMORY_COUNTERS pmc;
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
			wprintf(L" [Process] MaxWorkingSet : [%0.4f MB]\n", (double)pmc.PeakWorkingSetSize / (1024*1024)); // ���� ������ �� ����
			wprintf(L" [Process] MaxPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakPagedPoolUsage / (1024 * 1024)); // Paged-pool ��뷮 �ִ�ġ
			wprintf(L" [Process] MaxNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPeakNonPagedPoolUsage / (1024 * 1024)); //Non Paged-pool ��뷮 �ִ�ġ
			wprintf(L" [Process] maxPagefileUsage : [%0.4f MB]\n\n", (double)pmc.PeakPagefileUsage / (1024 * 1024)); // ���� �޸� Ŀ�Ե� ũ�� �ִ�ġ

			wprintf(L" [Process] CurrentWorkingSet : [%0.4f MB]\n", (double)pmc.WorkingSetSize / (1024 * 1024));  // ���μ����� ���������� ���ٰ�����, �޸𸮿� �����ϴ� �����ӵ�
			wprintf(L" [Process] CurrentPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaPagedPoolUsage / (1024 * 1024)); // ���� Paged-pool ��뷮
			wprintf(L" [Process] CurrentNonPagedPoolUsaged : [%0.4f MB]\n", (double)pmc.QuotaNonPagedPoolUsage / (1024 * 1024)); // ���� Non Paged-pool ��뷮
			wprintf(L" [Process] PageFileUsage : [%0.4f MB]\n", (double)pmc.PagefileUsage / (1024 * 1024)); // ���� ���� �޸� Ŀ�Ե� ũ��
		}

		wprintf(L"=================================================\n");
		wprintf(L"               SERVER MONITORING\n");
		wprintf(L"=================================================\n");
		wprintf(L" Press 'q' to shut down\n");
		wprintf(L"-------------------------------------------------\n");

		lobbyServer.Monitor();
		wprintf(L"-------------------------------------------------\n");
		wprintf(L" [Content] Total Users : %llu\n", jh_content::User::m_ullAliveLobbyUserCount);
		wprintf(L" [Content] Total Rooms : %d\n", jh_content::Room::GetAliveRoomCount());
		wprintf(L"=================================================\n\n\n");



		//LONG recvCount = InterlockedExchange(&m_lTotalRecvCount, 0);
		//LONG asyncRecvCount = InterlockedExchange(&m_lAsyncRecvCount, 0);

		//LONG sendCount = InterlockedExchange(&m_lTotalSendCount, 0);
		//LONG asyncSendCount = InterlockedExchange(&m_lAsyncSendCount, 0);

		//GetProcessTimes(GetCurrentProcess(), &processTimeInfo.creationTime, &processTimeInfo.exitTime, &processTimeInfo.kernnelTime, &processTimeInfo.userTime);

		//// ����ڰ� �б� ���� �ð����� ��ȯ
		//FileTimeToSystemTime(&processTimeInfo.creationTime, &processTimeInfo.systemTime);

		//wprintf(L"------------------------------------------------------------------------------------------\n");
		//wprintf(L"Server Start Time : %4d-%02d-%02d %2d:%2d:%2d \n", processTimeInfo.systemTime.wYear, processTimeInfo.systemTime.wMonth, processTimeInfo.systemTime.wDay, processTimeInfo.systemTime.wHour, processTimeInfo.systemTime.wMinute, processTimeInfo.systemTime.wSecond);
		//wprintf(L"Total Accepted Count [%lld]\n\n", m_llTotalAcceptedSessionCount);

		//wprintf(L"�����\tNetwork\t\t�����\n");
		//wprintf(L"Max Session Count [%u]\t\tCurrent Session Count [%u]\n\n", m_dwMaxSessionCnt, m_lSessionCount);

		//wprintf(L"Recv Count  [%d]\t\tAsync Recv Count [%d]\n", recvCount, asyncRecvCount);
		//wprintf(L"Send Count  [%d]\t\tAsync Send Count [%d]\n", sendCount, asyncSendCount);

		//wprintf(L"\n\n");
		//wprintf(L"�����\tContent\t\t�����\n");
		////wprintf(L"   Packet Pool Size [%lu], Use Size [%lu]\n", g_packetPool->GetCreationCount(), g_packetPool->GetAllocCount());
		//wprintf(L"      Job Pool Size [%lu], Use Size [%lu]\n", g_jobPool->GetCreationCount(), g_jobPool->GetAllocCount());
		//wprintf(L"SessionConnectionEvent Pool Size [%lu], Use Size [%lu]\n", g_systemJobPool->GetCreationCount(), g_systemJobPool->GetAllocCount());

		//ULONGLONG kT = ((ULONGLONG)processTimeInfo.kernnelTime.dwHighDateTime << 32) | processTimeInfo.kernnelTime.dwLowDateTime;
		//ULONGLONG uT = ((ULONGLONG)processTimeInfo.userTime.dwHighDateTime << 32) | processTimeInfo.userTime.dwLowDateTime;

		//// 100���� �� ���� 
		//// *10 => 1����ũ��
		//// *10 * 1000 -> 1�и�
		//// *10 * 1000 * 1000 -> 1��
		//wprintf(L"\n\n");
		//wprintf(L"�����\tProcessInfo\t�����\n");
		//wprintf(L"KernerTime\t: %0.5lfs\n", (kT - processTimeInfo.prevKernelTime) / (double)(1000 * 1000 * 10));
		//wprintf(L"UserTime\t: %0.5lfs\n", (uT - processTimeInfo.prevUserTime) / (double)(1000 * 1000 * 10));

		//wprintf(L"------------------------------------------------------------------------------------------\n");

		//processTimeInfo.prevKernelTime = kT;
		//processTimeInfo.prevUserTime = uT;

		Sleep(1000);
		//GetProcessTimes();
		//GetProcessMemoryInfo();
	}

	lobbyServer.Stop();
	
}

