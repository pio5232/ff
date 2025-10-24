// DummyClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <conio.h>
#include "LobbyDummyClient.h"

std::atomic<bool> reconnectFlag = true;
extern std::atomic<bool> clientSendFlag = true;
int main()
{
    jh_content::LobbyDummyClient dummyClient;

    SET_LOG_LEVEL(LOG_LEVEL_WARNING);

    dummyClient.Start();
    int maxSessionCnt = dummyClient.GetMaxSessionCount();
    //dummyClient.Connect(maxSessionCnt);

    ULONGLONG prevTime = jh_utility::GetTimeStamp();
    ULONGLONG curTime;

    while (1)
    {
        if (_kbhit())
        {
            char c = _getch();

            if ('Q' == c || 'q' == c)
                break;

            else if ('S' == c || 's' == c)
            {
                bool flag = clientSendFlag.load();
                clientSendFlag.store(!flag);
            }
            else if ('R' == c || 'r' == c)
            {
                bool flag = reconnectFlag.load();
                reconnectFlag.store(!flag);
            }
        }


        if (true == reconnectFlag.load())
        {
             int diff = maxSessionCnt - dummyClient.GetSessionCount();

            if (diff > 0)
            {
                int connectCount = diff > 100 ? 100 : diff;

                dummyClient.Connect(connectCount);

                Sleep(200);
            }
        }

        curTime = jh_utility::GetTimeStamp();
        if (curTime - prevTime > 1000)
        {
            wprintf(L"=================================================\n");
            wprintf(L"            DUMMY CLIENT MONITORING\n");
            wprintf(L"=================================================\n");
            wprintf(L" Press 'Q' to shut down\n");
            wprintf(L" Press 'R' to Reconnect, ReconnectMode    : [%s]\n", reconnectFlag ? L"YES" : L"NO");
            wprintf(L" Press 'S' to SendMode,  SendMode         : [%s]\n", clientSendFlag ? L"YES" : L"NO");
            wprintf(L"-------------------------------------------------\n");

            dummyClient.Monitor();

            prevTime = curTime;
        }
        //Sleep(1000);
    }

    dummyClient.Stop();
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
