#include "LibraryPch.h"
#include "CMonitor.h"

jh_utility::CMonitor::~CMonitor() 
{
}

void jh_utility::CMonitor::Start()
{
	
	_monitoringFlag = true;
	
	_monitorThread = std::thread([&]() {this->ProcessMonitoring(); });

	return;
}

void jh_utility::CMonitor::Stop()
{
	_monitoringFlag = false;

	if (_monitorThread.joinable())
		_monitorThread.join();

	return;
}

void jh_utility::CMonitor::ProcessMonitoring() 
{
	while (_monitoringFlag)
	{
		MonitoringJob();

		printf("\n");

		Sleep(1000);
	}
}
//
//void jh_utility::NetMonitor::MonitoringJob()
//{
//	while (_monitoringFlag)
//	{
//		printf("[ Current Connect Session Count : %u ]\n", _sessionMgr->GetSessionCnt());
//
//		//printf("[ SendPacket Completion Count : %u ]\n", InterlockedExchange(&_sendCount, 0));
//
//		//printf("[ Recv Completion Count : %u ]\n", InterlockedExchange(&_recvCount, 0));
//
//		printf("\n");
//
//		Sleep(1000);
//	}
//	return;
//}

