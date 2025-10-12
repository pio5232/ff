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
	//	// Decrease 전 IOCount
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
		// TODO : 함수 이름 변경하자
		// 각각의 함수들은 Start() / Stop()가 실행됐을 때
		// 상속받은 함수에서 추가적으로 작업할 것들을 여기에 등록하면 된다.
		virtual void BeginAction() {};
		virtual void EndAction() {};

		virtual bool OnConnectionRequest(const SOCKADDR_IN& clientInfo);
		virtual void OnError(int errCode, WCHAR* cause);

		// 포인터버전
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
		
		// session 수를 제외한 다른 옵션들을 설정.
		void InitServerConfig(WCHAR* ip, WORD port, DWORD concurrentWorkerThreadCount, WORD lingerOnOff, WORD lingerTime, ULONGLONG timeOut);
		
		Session* TryAcquireSession(ULONGLONG sessionId, const WCHAR* caller); // sessionPtr을 사용할 때마다 해제중인지 확인하고, 참조 카운트를 증가시킨다.

	private:

		void DeleteSession(ULONGLONG sessionId);
		void DecreaseIoCount(Session* sessionPtr);

		void WorkerThreadFunc();
		void AcceptThreadFunc();

		Session* CreateSession(SOCKET sock, const SOCKADDR_IN* pSockAddr);
		HANDLE m_hCompletionPort;
		std::vector<std::thread> m_workerThreads;

																// 설정 값
		WCHAR m_wszIp[IP_STRING_LEN]; // 원본 20				// ip
		USHORT m_usPort;											// 포트 번호
		DWORD m_dwMaxSessionCnt;								// 한번에 접속가능한 최대 세션 수
		DWORD m_dwConcurrentWorkerThreadCount;					// iocp에 등록할 worker 수
		LINGER m_lingerOption;									// TIME_OUT 옵션 설정 (GRACEFUL_SHUTDOWN 하지 않게 설정 위해.)
		ULONGLONG m_ullTimeOutLimit;							// HEARTBEAT

	private:
		const WCHAR* const m_pcwszServerName;					// 실행중인 서버의 이름 // Chatting / Lobby / Game.. 등등
		SOCKET m_listenSock;

		// Session의 할당 속도보다
		// Session을 Find하는 구조에서 병목이 적어야 한다는 게 내 생각
		jh_network::Session* m_pSessionArray;
		jh_utility::LockStack<DWORD> m_sessionIndexStack;

		ActiveSessionManager m_activeSessionManager;

		std::thread m_acceptThread;

		alignas(64) LONG m_lSessionCount;						// 현재 접속중인 세션 수
		alignas(64) LONGLONG m_llTotalAcceptedSessionCount;		// 시작부터 연결된 세션의 개수 

		alignas(64) LONGLONG m_llDisconnectedCount; // 상대쪽에서 연결을 끊은 횟수

		alignas(64) LONGLONG m_llTotalDisconnectedCount; // 상대 + 서버가 연결을 끊은 횟수

		alignas(64) LONG m_lTotalRecvCount;						// 1초 동기 + 비동기 RECV 수
		alignas(64) LONG m_lTotalSendCount;						// 1초 동기 + 비동기 SEND 수
		alignas(64) LONG m_lAsyncRecvCount;						// 1초 비동기 RECV 수
		alignas(64) LONG m_lAsyncSendCount;						// 1초 비동기 SEND 수


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

		void ForceStop(); // 강제 종료
		
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
	
		// 설정 값
		WCHAR m_wszTargetIp[IP_STRING_LEN]; // 원본 20				// ip
		USHORT m_usTargetPort;											// 포트 번호
		DWORD m_dwMaxSessionCnt;								// 한번에 접속가능한 최대 세션 수
		DWORD m_dwConcurrentWorkerThreadCount;					// iocp에 등록할 worker 수
		LINGER m_lingerOption;									// TIME_OUT 옵션 설정 (GRACEFUL_SHUTDOWN 하지 않게 설정 위해.)
		ULONGLONG m_ullTimeOutLimit;							// HEARTBEAT
	private:
		Session* TryAcquireSession(ULONGLONG sessionId); // sessionPtr을 사용할 때마다 해제중인지 확인하고, 참조 카운트를 증가시킨다.
		
		void DecreaseIoCount(Session* sessionPtr);
		void DeleteSession(ULONGLONG sessionId);

		
		Session* m_pClientSessionArr; // Client의 session은 IocpClient 인스턴스가 사라질 때 사라짐.
		jh_utility::LockStack<DWORD> m_sessionIndexStack;

		const NetAddress m_targetNetAddr;

		HANDLE m_hCompletionPort;
		HANDLE* m_hWorkerThreads;

		alignas(64) std::atomic<LONG> m_sessionCount;						// 현재 연결된 Session의 수.
		alignas(64) LONGLONG m_llTotalConnectedSessionCount;		// 시작부터 연결된 세션의 총 합

		alignas(64) LONG m_llDisconnectedCount; // 상대쪽에서 연결을 끊은 횟수
		alignas(64) LONGLONG m_llTotalDisconnectedCount; // 상대 + 서버가 연결을 끊은 횟수

		alignas(64) LONG m_lTotalRecvCount;						// 1초 동기 + 비동기 RECV 수
		alignas(64) LONG m_lTotalSendCount;						// 1초 동기 + 비동기 SEND 수
		alignas(64) LONG m_lAsyncRecvCount;						// 1초 비동기 RECV 수
		alignas(64) LONG m_lAsyncSendCount;						// 1초 비동기 SEND 수

	};

	
}