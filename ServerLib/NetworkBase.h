#pragma once
#include <unordered_map>
#include <iostream>
#include <functional>
#include <thread>
#include "Session.h"
#include "PacketDefine.h"
#include "CMonitor.h"

namespace jh_network
{
	/*-----------------------
			IocpServer
	-----------------------*/ 
	//enum SessionJob : ushort
	//{
	//	NONE = 0x00,
	//	RECV = 0x11,
	//	SEND = 0x22,
	//		
	//	TRANSFERRED_0 = 0xaa,
	//	DISCONNECT = 0xff,
	//	
	//};
	//struct SessionLog
	//{
	//	DWORD m_dwThreadId;
	//	// Decrease �� IOCount
	//	LONGLONG sessionIoCount;
	//	LONGLONG m_llSessionId;

	//	SessionJob sessionJob;
	//	
	//};
	class IocpServer
	{
	public:
		IocpServer(const WCHAR* serverName);
		virtual ~IocpServer() = 0;

		bool Start();
		void Listen();
		void Stop();

		void ForceStop();
		// TODO : �Լ� �̸� ��������
		// ������ �Լ����� Start() / Stop()�� ������� ��
		// ��ӹ��� �Լ����� �߰������� �۾��� �͵��� ���⿡ ����ϸ� �ȴ�.
		virtual void BeginAction() {};
		virtual void EndAction() {};

		virtual bool OnConnectionRequest(const SOCKADDR_IN& clientInfo);
		virtual void OnError(int errCode, WCHAR* cause);

		// �����͹���
		//virtual void OnRecv(LONGLONG sessionId, jh_utility::SerializationBuffer* dataBuffer, WORD type) = 0;
		virtual void OnRecv(ULONGLONG sessionId, PacketPtr dataBuffer, USHORT type) = 0;
		
		virtual void OnConnected(ULONGLONG sessionId) = 0;
		virtual void OnDisconnected(ULONGLONG sessionId) = 0;

		ErrorCode ProcessRecv(Session* sessionPtr, DWORD transferredBytes);
		ErrorCode ProcessSend(Session* sessionPtr, DWORD transferredBytes);

		void Disconnect(ULONGLONG sessionId, const WCHAR* reason, bool isCritical = false);

		void PostSend(Session* sessionPtr);
		void PostRecv(Session* sessionPtr);

		//void SendPacket(LONGLONG sessionId, jh_utility::SerializationBuffer* sendBuf);
		void SendPacket(ULONGLONG sessionId, PacketPtr& packet);

		void UpdateHeartbeat(ULONGLONG sessionId, ULONGLONG now);
		void CheckHeartbeatTimeout(ULONGLONG now);

		const WCHAR* const GetServerName() const { return m_pcwszServerName; }

		LONG GetSessionCount() const { return m_lSessionCount; }

		const WCHAR* GetIp() const { return m_wszIp; }

		USHORT GetPort() const;
		
		bool InitSessionArray(DWORD maxSessionCount);


	protected:

		LONG GetTotalRecvCount() { return InterlockedExchange(&m_lTotalRecvCount, 0); }
		LONG GetTotalSendCount() { return InterlockedExchange(&m_lTotalSendCount, 0); }
		LONG GetAsyncRecvCount() { return InterlockedExchange(&m_lAsyncRecvCount, 0); }
		LONG GetAsyncSendCount() { return InterlockedExchange(&m_lAsyncSendCount, 0); }

		LONGLONG GetDisconnectedCount() { return m_llDisconnectedCount; }
		LONGLONG GetTotalDisconnectedCount() { return m_llDisconnectedCount; }

		SOCKET GetListenSock() { return m_listenSock; }
		
		// session ���� ������ �ٸ� �ɼǵ��� ����.
		void InitServerConfig(WCHAR* ip, WORD port, DWORD concurrentWorkerThreadCount, WORD lingerOnOff, WORD lingerTime, ULONGLONG timeOut);
		
		Session* TryAcquireSession(ULONGLONG sessionId, const WCHAR* caller); // sessionPtr�� ����� ������ ���������� Ȯ���ϰ�, ���� ī��Ʈ�� ������Ų��.

	private:

		void DeleteSession(ULONGLONG sessionId);
		void DecreaseIoCount(Session* sessionPtr);

		void WorkerThreadFunc();
		void AcceptThreadFunc();

		Session* CreateSession(SOCKET sock, const SOCKADDR_IN* pSockAddr);
		HANDLE m_hCompletionPort;
		std::vector<std::thread> m_workerThreads;

																// ���� ��
		WCHAR m_wszIp[IP_STRING_LEN]; // ���� 20				// ip
		USHORT m_usPort;											// ��Ʈ ��ȣ
		DWORD m_dwMaxSessionCnt;								// �ѹ��� ���Ӱ����� �ִ� ���� ��
		DWORD m_dwConcurrentWorkerThreadCount;					// iocp�� ����� worker ��
		LINGER m_lingerOption;									// TIME_OUT �ɼ� ���� (GRACEFUL_SHUTDOWN ���� �ʰ� ���� ����.)
		ULONGLONG m_ullTimeOutLimit;							// HEARTBEAT

	private:
		const WCHAR* const m_pcwszServerName;					// �������� ������ �̸� // Chatting / Lobby / Game.. ���
		SOCKET m_listenSock;

		// Session�� �Ҵ� �ӵ�����
		// Session�� Find�ϴ� �������� ������ ����� �Ѵٴ� �� �� ����
		jh_network::Session* m_pSessionArray;
		jh_utility::LockStack<DWORD> m_sessionIndexStack;

		ActiveSessionManager m_activeSessionManager;

		std::thread m_acceptThread;

		alignas(64) LONG m_lSessionCount;						// ���� �������� ���� ��
		alignas(64) LONGLONG m_llTotalAcceptedSessionCount;		// ���ۺ��� ����� ������ ���� 

		alignas(64) LONGLONG m_llDisconnectedCount; // ����ʿ��� ������ ���� Ƚ��

		alignas(64) LONGLONG m_llTotalDisconnectedCount; // ��� + ������ ������ ���� Ƚ��

		alignas(64) LONG m_lTotalRecvCount;						// 1�� ���� + �񵿱� RECV ��
		alignas(64) LONG m_lTotalSendCount;						// 1�� ���� + �񵿱� SEND ��
		alignas(64) LONG m_lAsyncRecvCount;						// 1�� �񵿱� RECV ��
		alignas(64) LONG m_lAsyncSendCount;						// 1�� �񵿱� SEND ��


		// For Log
		//const LONG sessionLogMax = 200000;
		//LONG logIndex = 0;
		//void WriteSessionLog(LONGLONG m_llSessionId, LONGLONG m_lIoCount, SessionJob sessionJob);
		//SessionLog* _sessionLog;
	};

	class IocpClient
	{
	public:
		IocpClient(const WCHAR* clientName);
		virtual ~IocpClient() = 0;

		bool Start();
		void Stop();
	
		void Connect(int cnt);
		virtual void OnRecv(ULONGLONG sessionId, PacketPtr dataBuffer, USHORT type) = 0;

		virtual void OnConnected(ULONGLONG sessionId) = 0;
		virtual void OnDisconnected(ULONGLONG sessionId) = 0;

		void ProcessRecv(Session* sessionPtr, DWORD transferredBytes);
		void ProcessSend(Session* sessionPtr, DWORD transferredBytes);

		void Disconnect(ULONGLONG sessionId);

		void PostSend(Session* sessionPtr);
		void PostRecv(Session* sessionPtr);

		//void SendPacket(LONGLONG sessionId, jh_utility::SerializationBuffer* sendBuf);
		void SendPacket(ULONGLONG sessionId, PacketPtr& packet);

		static unsigned WINAPI WorkerThreadFunc(LPVOID lparam);

		void WorkerThread();

		Session* CreateSession(SOCKET sock, const SOCKADDR_IN* pSockAddr);
		
		void InitClientConfig(WCHAR* ip, WORD port, DWORD concurrentWorkerThreadCount, WORD lingerOnOff, WORD lingerTime, ULONGLONG timeOut);
		
		bool InitSessionArray(DWORD maxSessionCount);

		void ForceStop(); // ���� ����
		
		int GetMaxSessionCount() { return m_dwMaxSessionCnt; }
		LONG GetSessionCount() { return m_sessionCount.load(); }
		LONG GetTotalRecvCount() { return InterlockedExchange(&m_lTotalRecvCount, 0); }
		LONG GetTotalSendCount() { return InterlockedExchange(&m_lTotalSendCount, 0); }
		LONG GetAsyncRecvCount() { return InterlockedExchange(&m_lAsyncRecvCount, 0); }
		LONG GetAsyncSendCount() { return InterlockedExchange(&m_lAsyncSendCount, 0); }

		LONGLONG GetDisconnectedCount() { return m_llDisconnectedCount; }
		LONGLONG GetTotalDisconnectedCount() { return m_llDisconnectedCount; }
	protected:
		virtual void BeginAction() {}
		virtual void EndAction() {}
		const WCHAR* const m_pcwszClientName;
	
		// ���� ��
		WCHAR m_wszTargetIp[IP_STRING_LEN]; // ���� 20				// ip
		USHORT m_usTargetPort;											// ��Ʈ ��ȣ
		DWORD m_dwMaxSessionCnt;								// �ѹ��� ���Ӱ����� �ִ� ���� ��
		DWORD m_dwConcurrentWorkerThreadCount;					// iocp�� ����� worker ��
		LINGER m_lingerOption;									// TIME_OUT �ɼ� ���� (GRACEFUL_SHUTDOWN ���� �ʰ� ���� ����.)
		ULONGLONG m_ullTimeOutLimit;							// HEARTBEAT
	private:
		Session* TryAcquireSession(ULONGLONG sessionId); // sessionPtr�� ����� ������ ���������� Ȯ���ϰ�, ���� ī��Ʈ�� ������Ų��.
		
		void DecreaseIoCount(Session* sessionPtr);
		void DeleteSession(ULONGLONG sessionId);

		
		Session* m_pClientSessionArr; // Client�� session�� IocpClient �ν��Ͻ��� ����� �� �����.
		jh_utility::LockStack<DWORD> m_sessionIndexStack;

		const NetAddress m_targetNetAddr;

		HANDLE m_hCompletionPort;
		HANDLE* m_hWorkerThreads;

		alignas(64) std::atomic<LONG> m_sessionCount;						// ���� ����� Session�� ��.
		alignas(64) LONGLONG m_llTotalConnectedSessionCount;		// ���ۺ��� ����� ������ �� ��

		alignas(64) LONG m_llDisconnectedCount; // ����ʿ��� ������ ���� Ƚ��
		alignas(64) LONGLONG m_llTotalDisconnectedCount; // ��� + ������ ������ ���� Ƚ��

		alignas(64) LONG m_lTotalRecvCount;						// 1�� ���� + �񵿱� RECV ��
		alignas(64) LONG m_lTotalSendCount;						// 1�� ���� + �񵿱� SEND ��
		alignas(64) LONG m_lAsyncRecvCount;						// 1�� �񵿱� RECV ��
		alignas(64) LONG m_lAsyncSendCount;						// 1�� �񵿱� SEND ��

	};

	
}