#include "LibraryPch.h"
#include <MSWSock.h>
#include <Windows.h>
#include <conio.h>
#include "NetworkBase.h"

#include "ws2tcpip.h"
#include "Session.h"
#include "Memory.h"
#define ECHO

#pragma comment (lib, "ws2_32.lib")
	/*-----------------------
		  IocpServer
	-----------------------*/

jh_network::IocpServer::IocpServer(const WCHAR* serverName) : m_hCompletionPort(nullptr), m_listenSock(INVALID_SOCKET), m_pcwszServerName(serverName), m_pSessionArray(nullptr), m_wszIp{}
{
	m_dwConcurrentWorkerThreadCount = 0;
	m_lingerOption = {};
	m_dwMaxSessionCnt = 0;
	m_usPort = 0;
	m_ullTimeOutLimit = 0;

	//_sessionLog = new SessionLog[sessionLogMax];

	m_lSessionCount = 0;
	m_llTotalAcceptedSessionCount = 0;

	m_lAsyncRecvCount = 0;
	m_lAsyncSendCount = 0;
	
	m_lTotalRecvCount = 0;
	m_lTotalSendCount = 0;
	//m_packetPool = new jh_utility::LockFreeMemoryPool<jh_utility::SerializationBuffer>(0, true, true);

}

jh_network::IocpServer::~IocpServer() 
{
	//delete[] _sessionLog;

	if (nullptr != m_pSessionArray)
	{
		delete[] m_pSessionArray;
		m_pSessionArray = nullptr;
	}
}

bool jh_network::IocpServer::Start()
{
	DWORD concurrentThreadCnt = m_dwConcurrentWorkerThreadCount;

	if (0 == concurrentThreadCnt)
	{
		SYSTEM_INFO sys;
		GetSystemInfo(&sys);

		concurrentThreadCnt = sys.dwNumberOfProcessors;
	}

	_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Concurrent ThreadCount : %u", concurrentThreadCnt);

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, concurrentThreadCnt);

	if (m_hCompletionPort == NULL)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"IOCP Handle is Null");

		return false;
	}

	m_workerThreads.reserve(concurrentThreadCnt);

	for (int i = 0; i < concurrentThreadCnt; i++)
	{
		m_workerThreads.push_back(std::thread([this]() {this->WorkerThreadFunc(); }));
	}

	m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_listenSock == INVALID_SOCKET)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"ListenSocket is Invalid");

		return false;
	}

	
	SOCKADDR_IN sockaddrIn;
	sockaddrIn.sin_family = AF_INET;
	sockaddrIn.sin_port = htons(m_usPort);
	sockaddrIn.sin_addr = NetAddress::IpToAddr(m_wszIp);

	DWORD bindRet = bind(m_listenSock, (SOCKADDR*)&sockaddrIn, sizeof(SOCKADDR_IN));

	if (bindRet == SOCKET_ERROR)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Bind is Failed %d",getLastError);

		return false;
	}

	DWORD setLingerRet = setsockopt(m_listenSock, SOL_SOCKET, SO_LINGER, (char*)&m_lingerOption, sizeof(linger));
	if (setLingerRet == SOCKET_ERROR)
	{
		int getLastError = WSAGetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Set Linger Failed GetLastError : %i",getLastError);
		
		return false;
	}

	// TODO_
	m_acceptThread = std::thread([this]() { this->AcceptThreadFunc(); });

	BeginAction();

	return true;
}

void jh_network::IocpServer::Listen()
{
	DWORD listenRet = ::listen(m_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"[Listen] - Listen ����. ���� �ڵ� %u", getLastError);

		return;
	}
}

void jh_network::IocpServer::Stop()
{
	EndAction();

	closesocket(m_listenSock);

	m_listenSock = INVALID_SOCKET;

	if (m_acceptThread.joinable())
		m_acceptThread.join();

	for (int i = 0; i < m_workerThreads.size(); i++)
	{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, nullptr);
	}

	for (auto& t : m_workerThreads)
	{
		if (t.joinable())
			t.join();
	}
	
	if (nullptr != m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);

		m_hCompletionPort = nullptr;
	}


	ForceStop();

	return;
}

void jh_network::IocpServer::ForceStop()
{
	// �������� ���� ���ǵ��� ����
	for (int i = 0; i < m_dwMaxSessionCnt; i++)
	{
		Session* sessionPtr = &m_pSessionArray[i];

		if (INVALID_SOCKET == sessionPtr->m_socket)
			continue;

		LONGLONG sessionIdx = sessionPtr->m_ullSessionId >> SESSION_IDX_SHIFT_BIT;

		closesocket(sessionPtr->m_socket);
		sessionPtr->Reset();
		m_sessionIndexStack.Push(sessionIdx);

		InterlockedDecrement(&m_lSessionCount);
	}
	
}


void jh_network::IocpServer::DeleteSession(ULONGLONG sessionId)
{
	if (sessionId == INVALID_SESSION_ID)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Delete Session - TryAcquireSession Failed");
		return;
	}

	LONGLONG sessionIdx = sessionId >> SESSION_IDX_SHIFT_BIT;

	Session* sessionPtr = &m_pSessionArray[sessionIdx];
	// ������ �׳� ���� ���� �� ���� �� ������ �����Ǿ��� �� �ִ�. �׷��� ID Ȯ�� �� �ǳ��ֵ��� �Ѵ�.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		_LOG(L"Session", LOG_LEVEL_WARNING, L"Delete Session - Session ID != _SessionArray[SessionIdx].SessionID, SessionID : %lld, SessionIdx : %lld, called by %s", sessionId, sessionIdx, PROF_WFUNC);

		return;
	}

	int a;
	if (int a = InterlockedCompareExchange(&sessionPtr->m_lIoCount, SESSION_DELETE_FLAG, 0) != 0)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Delete SessION -> session is Using");
		return;
	}

	OnDisconnected(sessionId);

	m_activeSessionManager.RemoveActiveSession(sessionId);

	closesocket(sessionPtr->m_socket);

	sessionPtr->Reset();

	m_sessionIndexStack.Push(sessionIdx);

	InterlockedDecrement(&m_lSessionCount);
	_LOG(L"Session", LOG_LEVEL_DETAIL, L"Delete PushStack SessionIdx, SessionID : 0x%08x, SessionIdx : 0x%08x", sessionId, sessionIdx);

	// Delete�� ���� ���ǿ� ���ؼ� ioCount�� ���� �о������ ������ �������� �ʵ��� �Ѵ�..
	return;
}

void jh_network::IocpServer::DecreaseIoCount(Session* sessionPtr)
{
// Log
	LONG ioCount = InterlockedDecrement(&sessionPtr->m_lIoCount);
	
	//wprintf(L"After Decrease IOCount : %d, Job : 0x%02x     Ver2 \n", m_lIoCount);

	// DELETE�� �ϴ� ���� MSB�ʹ� ��� ���� ������� IOCount�� 0�� ��쿡 �ǽ��Ѵ�.
	//if (0 == (m_lIoCount & 0x7FFF'FFFF))
	//{
	//	DeleteSession(sessionPtr->m_llSessionId);
	//}
	if (0 == ioCount)
		DeleteSession(sessionPtr->m_ullSessionId);

}

//void jh_network::IocpServer::DecreaseIoCount(Session* sessionPtr, SessionJob job)
//{
//	LONG m_lIoCount = InterlockedDecrement(&sessionPtr->m_lIoCount);
//
//	WriteSessionLog(sessionPtr->m_llSessionId, m_lIoCount, job);
//	//wprintf(L"After Decrease IOCount : %d, Job : 0x%02x\n", m_lIoCount, job);
//	
//	if (0 == m_lIoCount)
//		DeleteSession(sessionPtr->m_llSessionId);
//
//}



void jh_network::IocpServer::InitServerConfig(WCHAR* ip, WORD port, DWORD concurrentWorkerThreadCount, WORD lingerOnOff, WORD lingerTime, ULONGLONG timeOut)
{
	wcscpy_s(m_wszIp, ip);
	m_usPort = port;
	m_dwConcurrentWorkerThreadCount = concurrentWorkerThreadCount;
	m_lingerOption.l_onoff = lingerOnOff;
	m_lingerOption.l_linger = lingerTime;

	m_ullTimeOutLimit = timeOut;
}

bool jh_network::IocpServer::InitSessionArray(DWORD maxSessionCount)
{
	if (m_pSessionArray != nullptr)
		return false;

	m_dwMaxSessionCnt = maxSessionCount;
	m_pSessionArray = new Session[m_dwMaxSessionCnt];

	m_sessionIndexStack.Reserve(m_dwMaxSessionCnt);

	for (int i = 0; i <m_dwMaxSessionCnt; i++)
	{
		m_sessionIndexStack.Push(i);
	}

	return true;
}

jh_network::Session* jh_network::IocpServer::TryAcquireSession(ULONGLONG sessionId, const WCHAR* caller)
{
	if (sessionId == INVALID_SESSION_ID)
	{
		_LOG(L"Session", LOG_LEVEL_WARNING, L"TryAcquireSession - Invalid Session ID [0x%0x], called by %s", sessionId, caller);
		return nullptr;
	}

	LONGLONG sessionIdx = sessionId >> SESSION_IDX_SHIFT_BIT;
	
	Session* sessionPtr = &m_pSessionArray[sessionIdx];
	// ������ �׳� ���� ���� �� ���� �� ������ �����Ǿ��� �� �ִ�. �׷��� ID Ȯ�� �� �ǳ��ֵ��� �Ѵ�.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		// �̹� ������ ���ǿ� �����ϴ� ���� ���� �������� ����.
		_LOG(L"Session", LOG_LEVEL_DEBUG, L"TryAcquireSession - Session ID != _SessionArray[SessionIdx].SessionID, SessionID : %lld, SessionIdx : %lld, called by %s", sessionId, sessionIdx, caller);

		return nullptr;
	}
	
	// ���� �Ŀ���. ���� ���ǿ� ���ؼ� ����ϱ� ���� �� �༮�� �������ΰ� �ƴѰ��� Ȯ���ϴ� ������ �ʿ��ϴ�.
	if (0 != (SESSION_DELETE_FLAG & InterlockedIncrement(&sessionPtr->m_lIoCount)))
	{
		DecreaseIoCount(sessionPtr);

		return nullptr;
	}
	//wprintf(L"TryAcquireSession ID [%lld], SINGLE THREAD - m_lIoCount : [%d], caller [%s]\n", m_llSessionId, sessionPtr->m_lIoCount, caller);

	return sessionPtr;
}

void jh_network::IocpServer::WorkerThreadFunc()
{
	while (1)
	{
		LPOVERLAPPED lpOverlapped = nullptr;
		DWORD transferredBytes = 0;
		ULONG_PTR key = 0;

		Session* sessionPtr = nullptr;

		bool gqcsRet = GetQueuedCompletionStatus(m_hCompletionPort, &transferredBytes,
			reinterpret_cast<PULONG_PTR>(&sessionPtr), &lpOverlapped, INFINITE);

		// 
		if (nullptr == lpOverlapped)
		{
			// gqcsRet == true -> pqcs�� ���� lpOverlapped �� nullptr
			// gqcsRet == false -> completionPort�� closeHandle�� ����.
			if (false == gqcsRet)
			{
				int wsaErr = WSAGetLastError();

				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"ProcessIO  Close Completion port [WSAERROR : %d]", wsaErr);
			}
			break;
		}

		// lpOverlapped != nullptr, gqcsRet == false
		// �۾� ���� ������ ������ ���� ��Ȳ�̴�.
		if (false == gqcsRet)
		{
			//DecreaseIoCount(sessionPtr);

			Disconnect(sessionPtr->m_ullSessionId, L"���� ����");

			DecreaseIoCount(sessionPtr);

			continue;
		}
		
		if (transferredBytes != 0)
		{
			if (&sessionPtr->m_recvOverlapped == lpOverlapped)
			{
				ProcessRecv(sessionPtr, transferredBytes);
			}
			else if (&sessionPtr->m_sendOverlapped == lpOverlapped)
			{
				ProcessSend(sessionPtr, transferredBytes);
			}
		}

		DecreaseIoCount(sessionPtr);
	}
}

bool jh_network::IocpServer::OnConnectionRequest(const SOCKADDR_IN& clientInfo)
{
	return true;
}


void jh_network::IocpServer::OnError(int errCode, WCHAR* cause)
{
}

ErrorCode jh_network::IocpServer::ProcessRecv(Session* sessionPtr, DWORD transferredBytes)
{

	if (false == sessionPtr->m_recvBuffer.MoveRearRetBool(transferredBytes))
	{
		Disconnect(sessionPtr->m_ullSessionId, L"ProcessRecv - Recv Buffer Overflow", true);

		return ErrorCode::RECV_BUF_OVERFLOW;
	}

	//jh_utility::SerializationBuffer m_pPacket(3000);
	//jh_network::SerializationBufferPtr m_pPacket = std::make_shared<jh_utility::SerializationBuffer>(512);

	while (1)
	{
		//m_pPacket.Clear();

		int bufferSize = sessionPtr->m_recvBuffer.GetUseSize();

		// packetheader���� ���� ����

#ifdef  ECHO
		if (bufferSize < sizeof(PacketHeader::size))
			break;

		USHORT header;

		sessionPtr->m_recvBuffer.PeekRetBool(reinterpret_cast<char*>(&header), sizeof(header));

		if (bufferSize < (sizeof(header) + header))
			break;

		sessionPtr->m_recvBuffer.MoveFront(sizeof(header));

		PacketPtr packet = MakeSharedBuffer(g_memAllocator, header);

		//jh_utility::SerializationBuffer* pPacket = g_packetPool->Alloc();

		//if (false == sessionPtr->m_recvBuffer.DequeueRetBool(pPacket->GetRearPtr(), header.size))
		if (false == sessionPtr->m_recvBuffer.DequeueRetBool(packet->GetRearPtr(), header))
		{
			Disconnect(sessionPtr->m_ullSessionId, L"ProcessRecv - Recv Buffer Deque Failed", true);

			return ErrorCode::RECV_BUF_DEQUE_FAILED;
		}

		//pPacket->MoveRearPos(header.size);
		packet->MoveRearPos(header);

		OnRecv(sessionPtr->m_ullSessionId, packet, 0);

#else
		if (bufferSize < sizeof(PacketHeader))
			break;

		PacketHeader header;

		sessionPtr->m_recvBuffer.PeekRetBool(reinterpret_cast<char*>(&header), sizeof(PacketHeader));

		if (bufferSize < (sizeof(PacketHeader) + header.size))
			break;

		sessionPtr->m_recvBuffer.MoveFront(sizeof(header));

		PacketPtr packet = MakeSharedBuffer(g_memAllocator, header.size);
		
		if (false == sessionPtr->m_recvBuffer.DequeueRetBool(packet->GetRearPtr(), header.size))
		{
			Disconnect(sessionPtr->m_ullSessionId, L"ProcessRecv - Recv Buffer Deque Failed", true);

			return ErrorCode::RECV_BUF_DEQUE_FAILED;
		}

		//pPacket->MoveRearPos(header.size);
		packet->MoveRearPos(header.size);

		OnRecv(sessionPtr->m_ullSessionId, packet, header.type);
#endif
	}

	PostRecv(sessionPtr);

	return ErrorCode::NONE;
}

ErrorCode jh_network::IocpServer::ProcessSend(Session* sessionPtr, DWORD transferredBytes)
{
	// ���� �׳� Clear���ְ�  �� ���ο��� Ǯ�� �ݳ��ϰ� ������, RefCount�� �����ϰ� �Ǵ� ���� �ٷ� �������ִ� ���� �ƴ�,
	// RefCount�� 0�� �Ǿ��� �� Pool�� �ݳ��ϵ��� �ؾ��Ѵ�.
	sessionPtr->m_sendOverlapped.ClearPendingList();

	//sessionPtr->m_sendBuffer.BufferLock();

	//if (false == sessionPtr->m_sendBuffer.MoveFrontRetBool(transferredBytes))
	//{
	//	_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"ProcessSend - Session SsendBuffer Move Front Error");
	//	jh_utility::CrashDump::Crash();
	//}
	//sessionPtr->m_sendBuffer.BufferUnlock();

	InterlockedExchange8(&sessionPtr->m_bSendFlag, 0);

	PostSend(sessionPtr);

	return ErrorCode::NONE;
}

void jh_network::IocpServer::Disconnect(ULONGLONG sessionId, const WCHAR* reason, bool isCritical)
{
	Session* sessionPtr = TryAcquireSession(sessionId, PROF_WFUNC);

	if (nullptr == sessionPtr)
		return;

	if(true == isCritical)
	_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Enter Disconnect Log - %s",reason);

	// ���� ���� ���°� �ƴ�, ���� ������ ���� ��Ȳ����
	// ��� �Ϸ� ������ ������ �� Disconnect()�� ȣ��ȴٸ�
	// �״��� �Ϸ� ������ ó���� �� i/o ����� ���´�.
	InterlockedExchange8(&sessionPtr->m_bConnectedFlag, 0);

	// recv, send�� ��ϵǾ��ٸ� �� �� io ����Ѵ�.
	CancelIoEx(reinterpret_cast<HANDLE>(sessionPtr->m_socket), nullptr);

	DecreaseIoCount(sessionPtr);
}

void jh_network::IocpServer::PostSend(Session* sessionPtr)
{
	if (InterlockedAnd8(&sessionPtr->m_bConnectedFlag, 1) == 0)
		return;

	if (InterlockedExchange8(&sessionPtr->m_bSendFlag, 1) == 1)
		return;

	// �տ��� ���� �͸����δ� ��� ���� �� ����.
	// 0���� �ƴ��� Ȯ���ϰ� ������ ������ 0���� �� �� ����.
	if (sessionPtr->m_sendQ.GetUseSize() == 0)
	{
		InterlockedExchange8(&sessionPtr->m_bSendFlag, 0);
		return;
	}

	//int wsabufCount = 1;
	//WSABUF wsabuf[2];

	//int totalUseSize = sessionPtr->m_sendBuffer.GetUseSize();
	//int directUseSize = sessionPtr->m_sendBuffer.DirectDequeueSize();

	//wsabuf[0].buf = sessionPtr->m_sendBuffer.GetFrontBufferPtr();
	//wsabuf[0].len = directUseSize;

	//if (totalUseSize - directUseSize > 0)
	//{
	//	wsabuf[1].buf = sessionPtr->m_sendBuffer.GetStartBufferPtr();
	//	wsabuf[1].len = totalUseSize - directUseSize;
	//	++wsabufCount;
	//}

	int popCount = sessionPtr->m_sendQ.PopAll(sessionPtr->m_sendOverlapped.m_pendingList);

	std::vector<WSABUF> wsaBufs;

	wsaBufs.reserve(popCount);

	for (PacketPtr& packet : sessionPtr->m_sendOverlapped.m_pendingList)
	{
		wsaBufs.push_back({static_cast<ULONG>(packet->GetDataSize()), packet->GetFrontPtr()});
		//wsaBufs[idx].buf = packet->GetFrontPtr();
		//wsaBufs[idx].len = packet->GetDataSize();
		//++idx;
	}

	InterlockedIncrement(&sessionPtr->m_lIoCount);

	int sendRet = WSASend(sessionPtr->m_socket, wsaBufs.data(), popCount, nullptr, 0, reinterpret_cast<LPWSAOVERLAPPED>(&sessionPtr->m_sendOverlapped), nullptr);

	if (sendRet == SOCKET_ERROR)
	{
		DWORD wsaErr = WSAGetLastError();

		// ��Ͽ� ������ ��Ȳ
		if (wsaErr != WSA_IO_PENDING)
		{
			//session->m_sendOverlapped.ownerSession = nullptr;

			switch (wsaErr)
			{
			case 10053:;

				// ����ڰ� �Ϲ������� ������ ���� ���� ���� ����� ���� �ʵ��� �ϰڴ�.  WSAECONNRESET
			case 10054:;
			default:
				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L" [PostSend]	���� �ڵ� : %u,  ���� ID : [0x%08x]", wsaErr, sessionPtr->m_ullSessionId);
				break;
			}

			//DecreaseIoCount(sessionPtr, SessionJob::SEND);
			DecreaseIoCount(sessionPtr);

			return;
		}
		InterlockedIncrement(&m_lAsyncSendCount);
	}
	InterlockedIncrement(&m_lTotalSendCount);
}

void jh_network::IocpServer::PostRecv(Session* sessionPtr)
{
	//if (1 != InterlockedAnd8(&sessionPtr->useFlag, 1))
	//{
	//	return;
	//}

	if (InterlockedAnd8(&sessionPtr->m_bConnectedFlag, 1) == 0)
		return;

	WSABUF buf[2];

	int directEnqueueSize = sessionPtr->m_recvBuffer.DirectEnqueueSize();
	int remainderSize = sessionPtr->m_recvBuffer.GetFreeSize() - directEnqueueSize;
	int wsabufSize = 1;

	buf[0].buf = sessionPtr->m_recvBuffer.GetFrontBufferPtr();
	buf[0].len = directEnqueueSize;

	if (remainderSize > 0)
	{
		++wsabufSize;
		buf[1].buf = sessionPtr->m_recvBuffer.GetStartBufferPtr();
		buf[1].len = remainderSize;
	}

	DWORD flag = 0;

	//if (false == TryIncreaseIoCount(sessionPtr))
	//	return;

	InterlockedIncrement(&sessionPtr->m_lIoCount);

	int recvRet = WSARecv(sessionPtr->m_socket, buf, wsabufSize, nullptr, &flag, reinterpret_cast<LPWSAOVERLAPPED>(&sessionPtr->m_recvOverlapped), nullptr);

	if (recvRet == SOCKET_ERROR)
	{
		int wsaErr = WSAGetLastError();

		if (wsaErr != WSA_IO_PENDING)
		{
			switch (wsaErr)
			{
				//
			case 10053:
				// ����ڰ� �Ϲ������� ������ ���� ���� ���� ����� ���� �ʵ��� �ϰڴ�. WSAECONNRESET
			case 10054:
			default:
				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L" [PostRecv]	���� �ڵ� : %u,  ���� ID : [0x%08x]", wsaErr, sessionPtr->m_ullSessionId);
				break;
			}

			//DecreaseIoCount(sessionPtr, SessionJob::RECV);
			DecreaseIoCount(sessionPtr);

			return;
		}

		InterlockedIncrement(&m_lAsyncRecvCount);
	}
	InterlockedIncrement(&m_lTotalRecvCount);
}

void jh_network::IocpServer::UpdateHeartbeat(ULONGLONG sessionId, ULONGLONG timeStamp)
{
	Session* sessionPtr = TryAcquireSession(sessionId, PROF_WFUNC);

	if (nullptr == sessionPtr)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Update Heartbeat - TryAcquireSession Failed, 0x%08x", sessionId);
		return;
	}
	if (timeStamp > sessionPtr->m_ullLastTimeStamp)
		sessionPtr->m_ullLastTimeStamp = timeStamp;

	DecreaseIoCount(sessionPtr);

}

void jh_network::IocpServer::CheckHeartbeatTimeout(ULONGLONG now)
{
	auto func = [this, now](ULONGLONG sessionId) {
		Session* sessionPtr = TryAcquireSession(sessionId, PROF_WFUNC);

		if (nullptr == sessionPtr)
		{
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"CheckHeartbeatTimeout - TryAcquireSession Failed, %0x", sessionId);
			return;
		}

		if (now - sessionPtr->m_ullLastTimeStamp > m_ullTimeOutLimit)
		{
			Disconnect(sessionPtr->m_ullSessionId, L"TIME OUT");
		}
		DecreaseIoCount(sessionPtr);
	};

	m_activeSessionManager.ProcessAllSessions(func);
}

// 0�� ���ϵǴ� ���� �߸��� ���
USHORT jh_network::IocpServer::GetPort() const
{
	if (0 != m_usPort && INVALID_SOCKET != m_listenSock)
		return m_usPort;

	return jh_network::NetAddress::GetPort(m_listenSock);
}

//void jh_network::IocpServer::SendPacket(LONGLONG sessionId, jh_utility::SerializationBuffer* packet)
void jh_network::IocpServer::SendPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	Session* sessionPtr = TryAcquireSession(sessionId, PROF_WFUNC);

	if (nullptr == sessionPtr)
	{
		//_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"SendPacket - TryAcquireSession Failed, %0x", sessionId);

		//Disconnect(m_llSessionId, L"SendPacket Disconnect");
		return;
	}

	sessionPtr->m_sendQ.Push(packet);

	PostSend(sessionPtr);

	DecreaseIoCount(sessionPtr);
}



void jh_network::IocpServer::AcceptThreadFunc()
{
	while (1)
	{
		SOCKADDR_IN clientInfo;
		int infoSize = sizeof(clientInfo);
		SOCKET clientSock = accept(m_listenSock, (SOCKADDR*)&clientInfo, &infoSize);

		// Accept ����
		if (clientSock == INVALID_SOCKET)
		{
			DWORD lastErr = WSAGetLastError();
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Accpet Thread - Client Socket is INVALID");
			break;
  		}
		if (!OnConnectionRequest(clientInfo)) // Client�� ������ ������ �� ���� ������ ���� ��
		{
			closesocket(clientSock);
			continue;
		}

		//_LOG(m_pcwszServerName, LOG_LEVEL_DETAIL, L"Enter Session, Current Session Count : %d",m_lSessionCount);
		Session* newSession = CreateSession(clientSock, &clientInfo);

		if (nullptr == newSession)
		{
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"[AcceptThreadFunc] ������ nullptr�Դϴ�.");

			closesocket(clientSock);
			continue;
		}
		
		OnConnected(newSession->m_ullSessionId);

		PostRecv(newSession);
	}
}

jh_network::Session* jh_network::IocpServer::CreateSession(SOCKET sock, const SOCKADDR_IN* pSockAddr)
{
	// ������� ���ǻ��� ������ ����ϴ°� ����,
	DWORD availableIndex = UINT_MAX;

	if (false == m_sessionIndexStack.TryPop(availableIndex))
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Create Session User Count Max... return Null, Count : %u",m_lSessionCount);

		return nullptr;
	}

	// ���� ���� �����ϰ�.
	InterlockedIncrement(&m_lSessionCount);
	Session* sessionPtr = &m_pSessionArray[availableIndex];

	// ���� �ʱ�ȭ
	static ULONG sessionIdGen = 0;
	
	//sessionPtr->m_socket = sock;
	//sessionPtr->m_targetNetAddr = *pSockAddr;
	//sessionPtr->m_ullLastTimeStamp = jh_utility::GetTimeStamp();

	// Session�� ����� id
	// [16 idx ][48 id]

	ULONGLONG id = (static_cast<ULONGLONG>(availableIndex) << SESSION_IDX_SHIFT_BIT) | ((++sessionIdGen) & SESSION_ID_MASKING_BIT);

	//sessionPtr->m_ullSessionId = id;

	sessionPtr->Activate(sock, pSockAddr, id);

	_LOG(L"Session", LOG_LEVEL_DETAIL, L"CreateSession -  Session Index : 0x%08x, Session ID : 0x%08x ", availableIndex, id);
	
	m_activeSessionManager.AddActiveSession(sessionPtr);

	// ���� ��� ����
	HANDLE ret = CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock), m_hCompletionPort, reinterpret_cast<ULONG_PTR>(sessionPtr), 0);
	if (nullptr == ret)
	{
		m_sessionIndexStack.Push(availableIndex);

		InterlockedDecrement(&m_lSessionCount);

		_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"CreateSession - Session Register Failed");

		return nullptr;
	}

	InterlockedIncrement64(&m_llTotalAcceptedSessionCount);
	
	return sessionPtr;
}

//void jh_network::IocpServer::WriteSessionLog(LONGLONG m_llSessionId, LONGLONG m_lIoCount, SessionJob sessionJob)
//{
//	DWORD threadId = GetCurrentThreadId();
//	LONG order = InterlockedIncrement(&logIndex) % sessionLogMax;
//
//	_sessionLog[order].threadId = threadId;
//	_sessionLog[order].m_llSessionId = m_llSessionId;
//	_sessionLog[order].sessionIoCount = m_lIoCount;
//	_sessionLog[order].sessionJob = sessionJob;
//
//}

// ----------------- IocpClient ----------------------

void jh_network::IocpClient::ProcessRecv(Session* sessionPtr, DWORD transferredBytes)
{
	if (false == sessionPtr->m_recvBuffer.MoveRearRetBool(transferredBytes))
	{
		Disconnect(sessionPtr->m_ullSessionId);

		return;
	}

	while (1)
	{
		int bufferSize = sessionPtr->m_recvBuffer.GetUseSize();

#ifdef ECHO
		if (bufferSize < sizeof(PacketHeader::size))
			break;

		USHORT header;

		sessionPtr->m_recvBuffer.PeekRetBool(reinterpret_cast<char*>(&header), sizeof(header));

		if (bufferSize < (sizeof(header) + header))
			break;

		sessionPtr->m_recvBuffer.MoveFront(sizeof(header));

		PacketPtr packet = MakeSharedBuffer(g_memAllocator, header);

		if (false == sessionPtr->m_recvBuffer.DequeueRetBool(packet->GetRearPtr(), header))
		{
			Disconnect(sessionPtr->m_ullSessionId);

			return;
		}

		packet->MoveRearPos(header);

		OnRecv(sessionPtr->m_ullSessionId, packet, 0);

#else
		if (bufferSize < sizeof(PacketHeader))
			break;

		PacketHeader header;

		sessionPtr->m_recvBuffer.PeekRetBool(reinterpret_cast<char*>(&header), sizeof(PacketHeader));

		if (bufferSize < (sizeof(PacketHeader) + header.size))
			break;

		sessionPtr->m_recvBuffer.MoveFront(sizeof(header));

		PacketPtr packet = MakeSharedBuffer(g_memAllocator, header.size);

		if (false == sessionPtr->m_recvBuffer.DequeueRetBool(packet->GetRearPtr(), header.size))
		{
			Disconnect(sessionPtr->m_ullSessionId);

			return;
		}

		packet->MoveRearPos(header.size);

		OnRecv(sessionPtr->m_ullSessionId, packet, header.type);
#endif
	}

	PostRecv(sessionPtr);
}

void jh_network::IocpClient::ProcessSend(Session* sessionPtr, DWORD transferredBytes)
{
	sessionPtr->m_sendOverlapped.ClearPendingList();

	InterlockedExchange8(&sessionPtr->m_bSendFlag, 0);

	PostSend(sessionPtr);

	return;
}

void jh_network::IocpClient::Disconnect(ULONGLONG sessionId)
{
	Session* sessionPtr = TryAcquireSession(sessionId);

	if (nullptr == sessionPtr)
		return;

	// ���� ���� ���°� �ƴ�, ���� ������ ���� ��Ȳ����
	// ��� �Ϸ� ������ ������ �� Disconnect()�� ȣ��ȴٸ�
	// �״��� �Ϸ� ������ ó���� �� i/o ����� ���´�.
	InterlockedExchange8(&sessionPtr->m_bConnectedFlag, 0);

	// recv, send�� ��ϵǾ��ٸ� �� �� io ����Ѵ�.
	CancelIoEx(reinterpret_cast<HANDLE>(sessionPtr->m_socket), nullptr);

	DecreaseIoCount(sessionPtr);
}

void jh_network::IocpClient::PostSend(Session* sessionPtr)
{
	if (InterlockedAnd8(&sessionPtr->m_bConnectedFlag, 1) == 0)
		return;

	if (InterlockedExchange8(&sessionPtr->m_bSendFlag, 1) == 1)
		return;

	// �տ��� ���� �͸����δ� ��� ���� �� ����.
	// 0���� �ƴ��� Ȯ���ϰ� ������ ������ 0���� �� �� ����.
	if (sessionPtr->m_sendQ.GetUseSize() == 0)
	{
		InterlockedExchange8(&sessionPtr->m_bSendFlag, 0);
		return;
	}

	int popCount = sessionPtr->m_sendQ.PopAll(sessionPtr->m_sendOverlapped.m_pendingList);

	std::vector<WSABUF> wsaBufs;

	wsaBufs.reserve(popCount);

	for (PacketPtr& packet : sessionPtr->m_sendOverlapped.m_pendingList)
	{
		wsaBufs.push_back({ static_cast<ULONG>(packet->GetDataSize()), packet->GetFrontPtr() });
	}

	InterlockedIncrement(&sessionPtr->m_lIoCount);

	int sendRet = WSASend(sessionPtr->m_socket, wsaBufs.data(), popCount, nullptr, 0, reinterpret_cast<LPWSAOVERLAPPED>(&sessionPtr->m_sendOverlapped), nullptr);

	if (sendRet == SOCKET_ERROR)
	{
		DWORD wsaErr = WSAGetLastError();

		// ��Ͽ� ������ ��Ȳ
		if (wsaErr != WSA_IO_PENDING)
		{
			switch (wsaErr)
			{
			case 10053:;

				// ����ڰ� �Ϲ������� ������ ���� ���� ���� ����� ���� �ʵ��� �ϰڴ�.  WSAECONNRESET
			case 10054:;
				break;
			default:
				_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"  SEND POST FAILED .. WSAERROR : %u, 0x%08x", wsaErr, sessionPtr->m_ullSessionId);
				break;
			}

			DecreaseIoCount(sessionPtr);

			return;
		}
		InterlockedIncrement(&m_lAsyncSendCount);
	}
	InterlockedIncrement(&m_lTotalSendCount);
}

void jh_network::IocpClient::PostRecv(Session* sessionPtr)
{
	if (InterlockedAnd8(&sessionPtr->m_bConnectedFlag, 1) == 0)
		return;

	WSABUF buf[2];

	int directEnqueueSize = sessionPtr->m_recvBuffer.DirectEnqueueSize();
	int remainderSize = sessionPtr->m_recvBuffer.GetFreeSize() - directEnqueueSize;
	int wsabufSize = 1;

	buf[0].buf = sessionPtr->m_recvBuffer.GetFrontBufferPtr();
	buf[0].len = directEnqueueSize;

	if (remainderSize > 0)
	{
		++wsabufSize;
		buf[1].buf = sessionPtr->m_recvBuffer.GetStartBufferPtr();
		buf[1].len = remainderSize;
	}

	DWORD flag = 0;

	//if (false == TryIncreaseIoCount(sessionPtr))
	//	return;

	InterlockedIncrement(&sessionPtr->m_lIoCount);

	int recvRet = WSARecv(sessionPtr->m_socket, buf, wsabufSize, nullptr, &flag, reinterpret_cast<LPWSAOVERLAPPED>(&sessionPtr->m_recvOverlapped), nullptr);

	if (recvRet == SOCKET_ERROR)
	{
		int wsaErr = WSAGetLastError();

		if (wsaErr != WSA_IO_PENDING)
		{
			switch (wsaErr)
			{
				//
			case 10053:
				// ����ڰ� �Ϲ������� ������ ���� ���� ���� ����� ���� �ʵ��� �ϰڴ�. WSAECONNRESET
			case 10054:
				break;
			default:
				_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[RecvPost] ���� WSAERROR : %u, 0x%08x", wsaErr, sessionPtr->m_ullSessionId);
				break;
			}

			//DecreaseIoCount(sessionPtr, SessionJob::RECV);
			DecreaseIoCount(sessionPtr);

			return;
		}

		InterlockedIncrement(&m_lAsyncRecvCount);
	}
	InterlockedIncrement(&m_lTotalRecvCount);
}

void jh_network::IocpClient::SendPacket(ULONGLONG sessionId, PacketPtr& packet)
{
	Session* sessionPtr = TryAcquireSession(sessionId);

	if (nullptr == sessionPtr)
		return;

	sessionPtr->m_sendQ.Push(packet);

	PostSend(sessionPtr);

	DecreaseIoCount(sessionPtr);
}

unsigned WINAPI jh_network::IocpClient::WorkerThreadFunc(LPVOID lparam)
{
	IocpClient* instance = reinterpret_cast<IocpClient*>(lparam);

	instance->WorkerThread();

	return 0;
}

void jh_network::IocpClient::WorkerThread()
{
	while (1)
	{
		LPOVERLAPPED lpOverlapped = nullptr;
		DWORD transferredBytes = 0;
		ULONG_PTR key = 0;

		Session* sessionPtr = nullptr;

		bool gqcsRet = GetQueuedCompletionStatus(m_hCompletionPort, &transferredBytes,
			reinterpret_cast<PULONG_PTR>(&sessionPtr), &lpOverlapped, INFINITE);

		// 
		if (nullptr == lpOverlapped)
		{
			// gqcsRet == true -> pqcs�� ���� lpOverlapped �� nullptr
			// gqcsRet == false -> completionPort�� closeHandle�� ����.
			if (false == gqcsRet)
			{
				int wsaErr = WSAGetLastError();

				_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"ProcessIO  Close Completion port [WSAERROR : %d]", wsaErr);
			}
			break;
		}

		// lpOverlapped != nullptr, gqcsRet == false
		// �۾� ���� ������ ������ ���� ��Ȳ�̴�.
		if (false == gqcsRet)
		{
			wprintf(L"-----------------------------------------------GQCSRET FALSE --------------------------------------------\n");
			Disconnect(sessionPtr->m_ullSessionId);

			DecreaseIoCount(sessionPtr);

			continue;
		}

		if (&sessionPtr->m_connectOverlapped == lpOverlapped)
		{
			OnConnected(sessionPtr->m_ullSessionId);

			// api����� ���� ���� ���� �ʱ�ȭ
			setsockopt(sessionPtr->m_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);

			PostRecv(sessionPtr);
		}
		else if (transferredBytes != 0)
		{
			if (&sessionPtr->m_recvOverlapped == lpOverlapped)
			{
				ProcessRecv(sessionPtr, transferredBytes);
			}
			else if (&sessionPtr->m_sendOverlapped == lpOverlapped)
			{
				ProcessSend(sessionPtr, transferredBytes);
			}
		}

		DecreaseIoCount(sessionPtr);
	}
}

jh_network::Session* jh_network::IocpClient::TryAcquireSession(ULONGLONG sessionId)
{
	if (sessionId == INVALID_SESSION_ID)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[TryAcquireSession] - ��ȿ���� ���� �����Դϴ�. Session ID [0x%0x]", sessionId);
		return nullptr;
	}

	LONGLONG sessionIdx = sessionId >> SESSION_IDX_SHIFT_BIT;

	Session* sessionPtr = &m_pClientSessionArr[sessionIdx];
	// ������ �׳� ���� ���� �� ���� �� ������ �����Ǿ��� �� �ִ�. �׷��� ID Ȯ�� �� �ǳ��ֵ��� �Ѵ�.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		// �̹� ������ ���ǿ� �����ϴ� ���� ���� �������� ����.
		_LOG(m_pcwszClientName, LOG_LEVEL_DEBUG, L"[TryAcquireSession] - ������ ������ ID�� �ٸ��ϴ�. SessionID : [%lld], SessionIdx : [%lld]", sessionId, sessionIdx);

		return nullptr;
	}

	// ���� �Ŀ���. ���� ���ǿ� ���ؼ� ����ϱ� ���� �� �༮�� �������ΰ� �ƴѰ��� Ȯ���ϴ� ������ �ʿ��ϴ�.
	if (0 != (SESSION_DELETE_FLAG & InterlockedIncrement(&sessionPtr->m_lIoCount)))
	{
		DecreaseIoCount(sessionPtr);

		return nullptr;
	}

	return sessionPtr;
}

void jh_network::IocpClient::DecreaseIoCount(Session* sessionPtr)
{
	// Log
	LONG ioCount = InterlockedDecrement(&sessionPtr->m_lIoCount);

	if (0 == ioCount)
		DeleteSession(sessionPtr->m_ullSessionId);

}


jh_network::Session* jh_network::IocpClient::CreateSession(SOCKET sock, const SOCKADDR_IN* pSockAddr)
{
	// ������� ���ǻ��� ������ ����ϴ°� ����,
	DWORD availableIndex = UINT_MAX;

	if (false == m_sessionIndexStack.TryPop(availableIndex))
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_INFO, L"[CreateSession] ����� �� �ִ� ������ �����ϴ�. ���� ���� �� : [%u]", m_lSessionCount);

		return nullptr;
	}

	// ���� ���� �����ϰ�.
	InterlockedIncrement(&m_lSessionCount);

	Session* sessionPtr = &m_pClientSessionArr[availableIndex];

	// ���� �ʱ�ȭ
	static ULONG sessionIdGen = 0;

	//sessionPtr->m_socket = sock;
	//sessionPtr->m_targetNetAddr = *pSockAddr;
	//sessionPtr->m_ullLastTimeStamp = jh_utility::GetTimeStamp();

	// Session�� ����� id
	// [16 idx ][48 id]

	ULONGLONG id = (static_cast<ULONGLONG>(availableIndex) << SESSION_IDX_SHIFT_BIT) | ((++sessionIdGen) & SESSION_ID_MASKING_BIT);

	//sessionPtr->m_ullSessionId = id;

	sessionPtr->Activate(sock, pSockAddr, id);

	_LOG(L"Session", LOG_LEVEL_DETAIL, L"CreateSession -  Session Index : 0x%08x, Session ID : 0x%08x ", availableIndex, id);

	// ���� ��� ����
	HANDLE ret = CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock), m_hCompletionPort, reinterpret_cast<ULONG_PTR>(sessionPtr), 0);
	if (nullptr == ret)
	{
		m_sessionIndexStack.Push(availableIndex);

		InterlockedDecrement(&m_lSessionCount);

		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"CreateSession - Session Register Failed");

		return nullptr;
	}

	InterlockedIncrement64(&m_llTotalConnectedSessionCount);

	return sessionPtr;

}

void jh_network::IocpClient::DeleteSession(ULONGLONG sessionId)
{
	if (sessionId == INVALID_SESSION_ID)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[DeleteSession] - ��ȿ���� ���� �����Դϴ�. Session Id : [%llu]",sessionId);
		return;
	}

	LONGLONG sessionIdx = sessionId >> SESSION_IDX_SHIFT_BIT;

	Session* sessionPtr = &m_pClientSessionArr[sessionIdx];
	// ������ �׳� ���� ���� �� ���� �� ������ �����Ǿ��� �� �ִ�. �׷��� ID Ȯ�� �� �ǳ��ֵ��� �Ѵ�.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[DeleteSession] - ������ ������ ID�� �ٸ��ϴ�. SessionID : [%lld], SessionIdx : [%lld]", sessionId, sessionIdx);

		return;
	}

	int a;
	if (int a = InterlockedCompareExchange(&sessionPtr->m_lIoCount, SESSION_DELETE_FLAG, 0) != 0)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_INFO, L" [DeleteSession] - ������� �����Դϴ�. SessionId : [%llu]", sessionId);
		return;
	}

	//OnDisconnected(sessionId);

	closesocket(sessionPtr->m_socket);

	sessionPtr->Reset();

	m_sessionIndexStack.Push(sessionIdx);

	InterlockedDecrement(&m_lSessionCount);

	_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[DeleteSession] - ������ �����Ǿ����ϴ�.. Session ID [0x%0x]", sessionId);

	//_LOG(L"Session", LOG_LEVEL_DETAIL, L"Delete PushStack SessionIdx, SessionID : 0x%08x, SessionIdx : 0x%08x", sessionId, sessionIdx);

	// Delete�� ���� ���ǿ� ���ؼ� ioCount�� ���� �о������ ������ �������� �ʵ��� �Ѵ�..
	return;
}

void jh_network::IocpClient::InitClientConfig(WCHAR* ip, WORD port, DWORD concurrentWorkerThreadCount, WORD lingerOnOff, WORD lingerTime, ULONGLONG timeOut)
{
	wcscpy_s(m_wszTargetIp, ip);
	m_usTargetPort = port;
	m_dwConcurrentWorkerThreadCount = concurrentWorkerThreadCount;
	m_lingerOption.l_onoff = lingerOnOff;
	m_lingerOption.l_linger = lingerTime;

	m_ullTimeOutLimit = timeOut;
}

bool jh_network::IocpClient::InitSessionArray(DWORD maxSessionCount)
{
	if (m_pClientSessionArr != nullptr)
		return false;

	m_dwMaxSessionCnt = maxSessionCount;
	m_pClientSessionArr = new Session[m_dwMaxSessionCnt];

	m_sessionIndexStack.Reserve(m_dwMaxSessionCnt);

	for (int i = 0; i < m_dwMaxSessionCnt; i++)
	{
		m_sessionIndexStack.Push(i);
	}

	return true;
}

void jh_network::IocpClient::ForceStop()
{
	// �������� ���� ���ǵ��� ����
	for (int i = 0; i < m_dwMaxSessionCnt; i++)
	{
		Session* sessionPtr = &m_pClientSessionArr[i];

		if (INVALID_SOCKET == sessionPtr->m_socket)
			continue;

		LONGLONG sessionIdx = sessionPtr->m_ullSessionId >> SESSION_IDX_SHIFT_BIT;

		closesocket(sessionPtr->m_socket);
		sessionPtr->Reset();
		m_sessionIndexStack.Push(sessionIdx);

		InterlockedDecrement(&m_lSessionCount);
	}
}

jh_network::IocpClient::IocpClient(const WCHAR* clientName) : m_pcwszClientName(clientName), m_hCompletionPort(nullptr), m_hWorkerThreads(nullptr), m_wszTargetIp{}, m_pClientSessionArr(nullptr), m_lingerOption{}
{
	m_dwConcurrentWorkerThreadCount = 0;
	m_dwMaxSessionCnt = 0;
	m_usTargetPort = 0;
	m_ullTimeOutLimit = 0;

	//_sessionLog = new SessionLog[sessionLogMax];

	m_lSessionCount = 0;
	m_llTotalConnectedSessionCount = 0;

	m_lAsyncRecvCount = 0;
	m_lAsyncSendCount = 0;
	m_lTotalRecvCount = 0;
	m_lTotalSendCount = 0;
}
jh_network::IocpClient::~IocpClient()
{

}

bool jh_network::IocpClient::Start()
{
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, m_dwConcurrentWorkerThreadCount);

	if (m_hCompletionPort == NULL)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Start] IOCP Handle�� Null�Դϴ�.");

		return false;
	}

	m_hWorkerThreads = new HANDLE[m_dwConcurrentWorkerThreadCount];

	for (int i = 0; i < m_dwConcurrentWorkerThreadCount; i++)
	{
		m_hWorkerThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, IocpClient::WorkerThreadFunc, this, 0, nullptr));

		if (nullptr == m_hWorkerThreads[i])
		{
			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Start] WorkerThread %d �� �������� �ʽ��ϴ�.", i);

			jh_utility::CrashDump::Crash();
		}
	}

	Connect();

	return true;
}

void jh_network::IocpClient::Stop()
{
	if (nullptr != m_hWorkerThreads)
	{
		for(int i =0 ;i< m_dwConcurrentWorkerThreadCount;i++)
		{
			PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, nullptr);
		}

		DWORD ret = WaitForMultipleObjects(m_dwConcurrentWorkerThreadCount, m_hWorkerThreads, true, INFINITE);
		
		for (int i = 0; i < m_dwConcurrentWorkerThreadCount; i++)
		{
			CloseHandle(m_hWorkerThreads[i]);
		}

		delete[] m_hWorkerThreads;
		m_hWorkerThreads = nullptr;
	}

	if (nullptr != m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);

		m_hCompletionPort = nullptr;
	}

	ForceStop();
}

void jh_network::IocpClient::Connect()
{
	SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == sock)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] ��ȿ���� ���� �����Դϴ�. GetLastError : [%d]", getLastError);

		closesocket(sock);
		return;
	}
	DWORD setLingerRet = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&m_lingerOption, sizeof(linger));

	if (SOCKET_ERROR == setLingerRet)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] SetLingerOpt ����. GetLastError : [%d]", getLastError);

		closesocket(sock);
		return;
	}
	// connectEx ��� �� bind �ʿ�
	sockaddr_in clientAddr{};
	clientAddr.sin_family = AF_INET;
	InetPton(AF_INET,m_wszTargetIp, &clientAddr.sin_addr);
	clientAddr.sin_port = htons(0);

	DWORD bindRet = bind(sock, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR_IN));

	if (SOCKET_ERROR == bindRet)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] Bind ����. GetLastError : [%d]", getLastError);

		closesocket(sock);

		return;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(m_usTargetPort);
	serverAddr.sin_addr = NetAddress::IpToAddr(m_wszTargetIp);

	Session* session = CreateSession(sock, &serverAddr);

	if (nullptr == session)
	{
		closesocket(sock);

		return;
	}

	//connect�� ���� ioCount ���� ���
	InterlockedIncrement(&session->m_lIoCount);
	
	DWORD bytes;

	bool retConnectEx = jh_network::NetAddress::lpfnConnectEx(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr), nullptr, 0, &bytes, &session->m_connectOverlapped);

	if (false == retConnectEx)
	{
		int wsaGetLastError = WSAGetLastError();

		if (WSA_IO_PENDING != wsaGetLastError)
		{
			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] ConnectEx ����. Session ID : [%llu]", session->m_ullSessionId);

			DecreaseIoCount(session);
		}
	}

	//if(jh_network::NetAddress::lpfnConnectEx)
	// TODO_
}
