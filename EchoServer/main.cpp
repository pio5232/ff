#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <conio.h>
#include <Psapi.h>
#include <crtdbg.h>
#include "EchoServer.h"

// C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.38.33130\include
int main()
{
	SET_LOG_LEVEL(LOG_LEVEL_INFO);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	jh_content::EchoServer echoServer;

	if (ErrorCode::NONE != echoServer.Start())
	{
		printf("EchoServer Begin() Return != NONE");
		return 0;
	}

	echoServer.Listen();
	while (1)
	{
		if (_kbhit())
		{
			char c = _getch();

			if (c == 'Q' || c == 'q')
				break;
		}
	
		system("cls");

		wprintf(L"=================================================\n");
		wprintf(L"               SERVER MONITORING\n");
		wprintf(L"=================================================\n");
		wprintf(L" Press 'q' to shut down\n");
		wprintf(L"-------------------------------------------------\n");

		echoServer.Monitor();
		wprintf(L"=================================================\n");


		Sleep(1000);
		//GetProcessTimes();
		//GetProcessMemoryInfo();
	}
	

	if (ErrorCode::NONE != echoServer.Stop())
	{
		printf("EchoServer End() Return != NONE");
		
		return 0;
	}

	_CrtDumpMemoryLeaks();

}