#pragma once

namespace jh_utility
{
	class CMonitor
	{
	public: CMonitor() {}
		virtual ~CMonitor() = 0;

		virtual void ProcessMonitoring();
		virtual void MonitoringJob() abstract = 0; // ��ӹ��� Ŭ�������� ����͸� �� �۾��� �Ѵ�.
		void Start();
		void Stop();

	protected:
		std::thread _monitorThread;
		volatile bool _monitoringFlag = false;

	};

	//class NetMonitor : public CMonitor
	//{
	//public:
	//	NetMonitor(class jh_network::SessionManager* sessionMgr) : _sessionMgr(sessionMgr) {}

	//	virtual void MonitoringJob() override;

	//	ULONG IncSendCount() { return InterlockedIncrement(&_sendCount); }
	//	ULONG IncRecvCount() { return InterlockedIncrement(&_recvCount); }
	//private:		
	//	class jh_network::SessionManager* const _sessionMgr;

	//	volatile ULONG _sendCount = 0; // 1�ʿ� �� ���� send �Ϸ�����ó�� / recv �Ϸ����� ó�� üũ,
	//	volatile ULONG _recvCount = 0;
	//};

	//class ChatMonitor : public CMonitor
	//{
	//public:
	//	ChatMonitor(class jh_network::RoomManager* roomMgr, class jh_network::UserManager* userMgr) : _roomMgr(roomMgr), _userMgr(userMgr) {}
	//	virtual void MonitoringJob() override;
	//private:
	//	class jh_network::RoomManager* _roomMgr;
	//	class jh_network::UserManager* _userMgr;
	//};

}
