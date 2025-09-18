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
	m_wPort = 0;
	m_ullTimeOutLimit = 0;

	//_sessionLog = new SessionLog[sessionLogMax];

	m_lSessionCount = 0;
	m_llTotalAcceptedSessionCount = 0;

	//m_packetPool = new jh_utility::LockFreeMemoryPool<jh_utility::SerializationBuffer>(0, true, true);

}

jh_network::IocpServer::~IocpServer() 
{
	//delete[] _sessionLog;

	if (nullptr != m_pSessionArray)
	{
		for (int i = 0; i < m_dwMaxSessionCnt; i++)
		{
			if (INVALID_SESSION_ID != m_pSessionArray[i].m_ullSessionId)
			{
				Disconnect(m_pSessionArray[i].m_ullSessionId, L"Server Close");
			}
		}
		delete[] m_pSessionArray;
		m_pSessionArray = nullptr;
	}


	WSACleanup();
}

ErrorCode jh_network::IocpServer::Start()
{
	WSAData wsa;

	int iRet = WSAStartup(MAKEWORD(2, 2), &wsa);

	if (iRet)
	{
		return ErrorCode::WSA_START_UP_ERROR;
	}

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

		return 	ErrorCode::CREATE_COMPLETION_PORT_FAILED;
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

		return ErrorCode::CREATE_SOCKET_FAILED;
	}

	DWORD setLingerRet = setsockopt(m_listenSock, SOL_SOCKET, SO_LINGER, (char*)&m_lingerOption, sizeof(linger));
	if (setLingerRet == SOCKET_ERROR)
	{
		int getLastError = WSAGetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Set Linger Failed GetLastError : %i",getLastError);
		
		return ErrorCode::SET_SOCK_OPT_FAILED;
	}
	
	SOCKADDR_IN sockaddrIn;
	sockaddrIn.sin_family = AF_INET;
	sockaddrIn.sin_port = htons(m_wPort);
	sockaddrIn.sin_addr = NetAddress::IpToAddr(m_wszIp);

	DWORD bindRet = bind(m_listenSock, (SOCKADDR*)&sockaddrIn, sizeof(SOCKADDR_IN));

	if (bindRet == SOCKET_ERROR)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Bind is Failed %d",getLastError);

		return ErrorCode::BIND_FAILED;

	}

	// TODO_
	m_acceptThread = std::thread([this]() { this->AcceptThreadFunc(); });

	BeginAction();

	return ErrorCode::NONE;
}

void jh_network::IocpServer::Listen()
{
	DWORD listenRet = ::listen(m_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"[Listen] - Listen 실패. 에러 코드 %u", getLastError);

		return;
	}
}

ErrorCode jh_network::IocpServer::Stop()
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
	
	CloseHandle(m_hCompletionPort);

	return ErrorCode::NONE;
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
	// 세션을 그냥 얻어가면 줬을 때 내가 준 세션이 해제되었을 수 있다. 그래서 ID 확인 후 건네주도록 한다.
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

	// Delete는 얻어온 세션에 대해서 ioCount를 새로 밀어버리기 때문에 정리하지 않도록 한다..
	return;
}

void jh_network::IocpServer::DecreaseIoCount(Session* sessionPtr)
{
// Log
	LONG ioCount = InterlockedDecrement(&sessionPtr->m_lIoCount);
	
	//wprintf(L"After Decrease IOCount : %d, Job : 0x%02x     Ver2 \n", m_lIoCount);

	// DELETE를 하는 것은 MSB와는 상관 없이 사용중인 IOCount가 0일 경우에 실시한다.
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
	m_wPort = port;
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
	// 세션을 그냥 얻어가면 줬을 때 내가 준 세션이 해제되었을 수 있다. 그래서 ID 확인 후 건네주도록 한다.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		// 이미 해제된 세션에 접근하는 것은 아주 정상적인 상태.
		_LOG(L"Session", LOG_LEVEL_DEBUG, L"TryAcquireSession - Session ID != _SessionArray[SessionIdx].SessionID, SessionID : %lld, SessionIdx : %lld, called by %s", sessionId, sessionIdx, caller);

		return nullptr;
	}
	
	// 받은 후에도. 받은 세션에 대해서 사용하기 전에 이 녀석이 정리중인가 아닌가를 확인하는 과정이 필요하다.
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
			// gqcsRet == true -> pqcs로 보낸 lpOverlapped 가 nullptr
			// gqcsRet == false -> completionPort가 closeHandle된 상태.
			if (false == gqcsRet)
			{
				int wsaErr = WSAGetLastError();

				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"ProcessIO  Close Completion port [WSAERROR : %d]", wsaErr);
			}
			break;
		}

		// lpOverlapped != nullptr, gqcsRet == false
		// 작업 도중 연결이 끊겼을 때의 상황이다.
		if (false == gqcsRet)
		{
			//DecreaseIoCount(sessionPtr);

			Disconnect(sessionPtr->m_ullSessionId, L"연결 끊김");

			DecreaseIoCount(sessionPtr);

			continue;
		}
		
		if (transferredBytes != 0)
		{
			if (&sessionPtr->m_recvOverlapped == lpOverlapped)
			{
				//sessionJob = SessionJob::RECV;

				ProcessRecv(sessionPtr, transferredBytes);
			}
			else if (&sessionPtr->m_sendOverlapped == lpOverlapped)
			{
				//sessionJob = SessionJob::SEND;
				//std::cout << "[WorkerThread] SessionID : " << sessionPtr->m_ullSessionId << " Send Completed Bytes : " << transferredBytes << "\n";
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

		// packetheader보다 작은 상태

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
	// 현재 그냥 Clear해주고  이 내부에선 풀에 반납하고 있지만, RefCount로 관리하게 되는 경우는 바로 해제해주는 것이 아닌,
	// RefCount가 0이 되었을 때 Pool에 반납하도록 해야한다.
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
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Disconnect - TryAcquireSession Failed");
		return;
	}

	if(true == isCritical)
	_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Enter Disconnect Log - %s",reason);

	// 연결 끊긴 상태가 아닌, 먼저 연결을 끊는 상황에서
	// 모두 완료 통지가 들어왔을 때 Disconnect()가 호출된다면
	// 그다음 완료 통지를 처리할 때 i/o 등록을 막는다.
	InterlockedExchange8(&sessionPtr->m_bConnectedFlag, 0);

	// recv, send가 등록되었다면 둘 다 io 취소한다.
	CancelIoEx(reinterpret_cast<HANDLE>(sessionPtr->m_socket), nullptr);

	DecreaseIoCount(sessionPtr);
}

void jh_network::IocpServer::PostSend(Session* sessionPtr)
{
	if (InterlockedAnd8(&sessionPtr->m_bConnectedFlag, 1) == 0)
		return;

	if (InterlockedExchange8(&sessionPtr->m_bSendFlag, 1) == 1)
		return;

	// 앞에서 막는 것만으로는 모두 막을 수 없음.
	// 0개가 아님을 확인하고 들어오는 순간에 0개가 될 수 있음.
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

		// 등록에 실패한 상황
		if (wsaErr != WSA_IO_PENDING)
		{
			//session->m_sendOverlapped.ownerSession = nullptr;

			switch (wsaErr)
			{
			case 10053:;

				// 사용자가 일방적으로 연결을 끊은 경우는 에러 출력을 하지 않도록 하겠다.  WSAECONNRESET
			case 10054:;
				break;
			default:
				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"  SEND POST FAILED .. WSAERROR : %u, 0x%08x", wsaErr, sessionPtr->m_ullSessionId);
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
				// 사용자가 일방적으로 연결을 끊은 경우는 에러 출력을 하지 않도록 하겠다. WSAECONNRESET
			case 10054:
				break;
			default:
				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"RECV POST FAILED .. WSAERROR : %u, 0x%08x", wsaErr, sessionPtr->m_ullSessionId);
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

//void jh_network::IocpServer::SendPacket(LONGLONG sessionId, jh_utility::SerializationBuffer* packet)
void jh_network::IocpServer::SendPacket(ULONGLONG sessionId, PacketPtr packet)
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

		// Accept 종료
		if (clientSock == INVALID_SOCKET)
		{
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"Accpet Thread - Client Socket is INVALID");
			break;
  		}
		if (!OnConnectionRequest(clientInfo)) // Client가 서버에 접속할 수 없는 이유가 있을 때
		{
			closesocket(clientSock);
			continue;
		}

		//_LOG(m_pcwszServerName, LOG_LEVEL_DETAIL, L"Enter Session, Current Session Count : %d",m_lSessionCount);
		Session* newSession = CreateSession(clientSock, &clientInfo);

		if (nullptr == newSession)
		{
			_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Created Session is Nullptr");

			closesocket(clientSock);
			continue;
		}
		
		OnConnected(newSession->m_ullSessionId);

		PostRecv(newSession);
	}
}

jh_network::Session* jh_network::IocpServer::CreateSession(SOCKET sock, const SOCKADDR_IN* pSockAddr)
{
	// 여기부터 세션빼서 내껄로 사용하는거 적기,
	DWORD availableIndex = UINT_MAX;

	if (false == m_sessionIndexStack.TryPop(availableIndex))
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Create Session User Count Max... return Null, Count : %u",m_lSessionCount);

		return nullptr;
	}

	// 세션 수를 증가하고.
	InterlockedIncrement(&m_lSessionCount);
	Session* sessionPtr = &m_pSessionArray[availableIndex];

	// 세션 초기화
	static ULONG sessionIdGen = 0;
	
	//sessionPtr->m_socket = sock;
	//sessionPtr->m_targetNetAddr = *pSockAddr;
	//sessionPtr->m_ullLastTimeStamp = jh_utility::GetTimeStamp();

	// Session에 등록할 id
	// [16 idx ][48 id]

	ULONGLONG id = (static_cast<ULONGLONG>(availableIndex) << SESSION_IDX_SHIFT_BIT) | ((++sessionIdGen) & SESSION_ID_MASKING_BIT);

	//sessionPtr->m_ullSessionId = id;

	sessionPtr->Activate(sock, pSockAddr, id);

	_LOG(L"Session", LOG_LEVEL_DETAIL, L"CreateSession -  Session Index : 0x%08x, Session ID : 0x%08x ", availableIndex, id);
	
	m_activeSessionManager.AddActiveSession(sessionPtr);

	// 세션 등록 실패
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

//jh_network::IocpClient::IocpClient(const NetAddress& m_targetNetAddr) : m_hCompletionPort(nullptr), _clientSession(nullptr),m_targetNetAddr(m_targetNetAddr)
//{
//	WSAData wsa;
//
//	int iRet = WSAStartup(MAKEWORD(2, 2), &wsa);
//
//	if (iRet)
//	{
//		_LOG(L"IocpClient", LOG_LEVEL_WARNING, L"[WSAStartUp Error !!]");
//		return;
//	}
//
//	SYSTEM_INFO sys;
//	GetSystemInfo(&sys);
//
//	DWORD concurrentThreadCnt = 1;
//
//	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, concurrentThreadCnt);
//
//	if (m_hCompletionPort == NULL)
//	{
//		_LOG(L"IocpClient", LOG_LEVEL_WARNING, L"[Create IOCP Handle is Failed!!]");
//		
//		return;
//	}
//
//
//	_workerThread = std::thread([this]() {this->WorkerThread(); });
//
//	SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
//	if (clientSock == INVALID_SOCKET)
//	{
//		_LOG(L"IocpClient", LOG_LEVEL_WARNING, L"[ClientSocket is INVALID]");
//
//		return;
//	}
//
//	_clientSession = new Session();
//	
//	static LONGLONG sessionIdGen = 0;
//	_clientSession->sessionIndex = 0;
//	_clientSession->m_llSessionId = ++sessionIdGen;
//	_clientSession->Init(clientSock, &m_targetNetAddr.GetSockAddr());
//
//	LINGER linger;
//	linger.l_linger = 0;
//	linger.l_onoff = 0;
//
//	DWORD setLingerRet = setsockopt(_clientSession->m_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
//	if (setLingerRet == SOCKET_ERROR)
//	{
//		DWORD getlast = GetLastError();
//
//		_LOG(L"IocpClient", LOG_LEVEL_WARNING, L"Set Linger Failed GetLastError : %u", getlast);
//
//		return;
//	}
//}
//
//jh_network::IocpClient::~IocpClient()
//{
//	if (INVALID_SESSION_INDEX != _clientSession->sessionIndex)
//	{
//		closesocket(_clientSession->m_socket);
//		_clientSession->Reset();
//	}
//	if(m_hCompletionPort != nullptr)
//	CloseHandle(m_hCompletionPort);
//
//	if (_workerThread.joinable())
//		_workerThread.join();
//
//	WSACleanup();
//}
//
//bool jh_network::IocpClient::Connect()
//{
//	if (_clientSession->m_socket == INVALID_SOCKET)
//	{
//		printf("[Socket is Invalid]");
//		return false;
//	}
//
//	int connectRet = connect(_clientSession->m_socket, (sockaddr*)&m_targetNetAddr.GetSockAddr(), sizeof(SOCKADDR_IN));
//
//	if (connectRet == SOCKET_ERROR)
//	{
//		printf("[Client Connect Error - %d", GetLastError());;
//		return false;
//	}
//
//	HANDLE RegistIocpRet = CreateIoCompletionPort((HANDLE)_clientSession->m_socket, m_hCompletionPort, NULL, NULL);
//
//	if (nullptr == RegistIocpRet)
//	{
//		printf("[Regist Session Handle Failed]\n");
//		return false;
//	}
//
//	OnConnected();
//
//	PostRecv();
//	//PostRecv();
//
//	return true;
//}
//
//
//bool jh_network::IocpClient::Disconnect(const WCHAR* reason, bool isCritical = false)
//{
//	char isDisconnected = InterlockedExchange(&_clientSession->_isDisconnected, 1);
//
//	if (0 == isDisconnected)
//	{
//		if (_clientSession->m_socket != INVALID_SOCKET)
//		{
//			if (true == isCritical)
//				_LOG(_clientName, LOG_LEVEL_WARNING, L"Critical Disconnect Session %llu, Reason : %s", _clientSession->m_llSessionId, reason);
//			else
//				_LOG(_clientName, LOG_LEVEL_INFO, L"Disconnect Session %llu, Reason : %s", _clientSession->m_llSessionId, reason);
//
//			closesocket(_clientSession->m_socket);
//
//			_clientSession->m_socket = INVALID_SOCKET;
//		}
//
//		OnDisconnected();
//	}
//
//	return false;
//}
//
//void jh_network::IocpClient::Send(jh_network::SerializationBufferPtr buffer)
//{
//	if (nullptr == _clientSession)
//	{
//		_LOG(_clientName, LOG_LEVEL_WARNING, L"Send - ClientSession is Null");
//		return;
//	}
//	
//	{
//		SRWLockGuard lockGuard(&_clientSession->sendBufferLock);
//		_clientSession->sendBufferQ.push(buffer);
//	}
//
//	PostSend();
//}
//
//bool jh_network::IocpClient::ProcessIO(DWORD timeOut)
//{
//	IocpEvent* iocpEvent;
//	DWORD transferredBytes;
//	ULONG_PTR key;
//	// Session*를 Key로 등록해서 가지는 형태도 사용할 수 있지만.
//	// 현재 작업 (IocpEvent)에 자신의 소유자 (Session*)가 등록되어 있기 때문에 Key는 사용하지 않는다.
//
//	iocpEvent = nullptr;
//	transferredBytes = 0;
//
//	key = 0;
//
//	bool gqcsRet = GetQueuedCompletionStatus(m_hCompletionPort, &transferredBytes,
//		&key, reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeOut);
//
//	if (!iocpEvent)
//	{
//		DWORD wsaErr = WSAGetLastError();
//
//		printf("IocpEvent is NULL\n");
//		if (wsaErr == WSA_WAIT_TIMEOUT)
//			return true;
//
//		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, nullptr);
//		return false;
//	}
//
//	Dispatch(iocpEvent, transferredBytes);
//
//	return true;
//}
//
//
//void jh_network::IocpClient::Dispatch(IocpEvent* iocpEvent, DWORD transferredBytes)
//{
//	switch (iocpEvent->iocpEventType)
//	{
//	case IocpEventType::Recv:
//	{
//		ProcessRecv(transferredBytes);
//
//		break;
//	}
//
//	case IocpEventType::Send:
//	{
//		ProcessSend(transferredBytes);
//
//		break;
//	}
//	default:break;
//	}
//}
//
//void jh_network::IocpClient::WorkerThread()
//{
//	while (1)
//	{
//		if (false == ProcessIO())
//			break;
//	}
//
//}
//
//void jh_network::IocpClient::PostSend()
//{
//	if (InterlockedExchange8(&sessionPtr->sendFlag, 1) == 1)
//		return;
//
//	// 앞에서 막는 것만으로는 모두 막을 수 없음.
//	// 0개가 아님을 확인하고 들어오는 순간에 0개가 될 수 있음.
//	if (sessionPtr->m_sendQ.GetUseSize() == 0)
//	{
//		InterlockedExchange8(&sessionPtr->sendFlag, 0);
//		return;
//	}
//
//	//int wsabufCount = 1;
//	//WSABUF wsabuf[2];
//
//	//int totalUseSize = sessionPtr->m_sendBuffer.GetUseSize();
//	//int directUseSize = sessionPtr->m_sendBuffer.DirectDequeueSize();
//
//	//wsabuf[0].buf = sessionPtr->m_sendBuffer.GetFrontBufferPtr();
//	//wsabuf[0].len = directUseSize;
//
//	//if (totalUseSize - directUseSize > 0)
//	//{
//	//	wsabuf[1].buf = sessionPtr->m_sendBuffer.GetStartBufferPtr();
//	//	wsabuf[1].len = totalUseSize - directUseSize;
//	//	++wsabufCount;
//	//}
//
//	int popCount = sessionPtr->m_sendQ.PopAll(sessionPtr->m_sendOverlapped.m_pendingList);
//
//	std::vector<WSABUF> wsaBufs;
//
//	wsaBufs.reserve(popCount);
//
//	for (PacketPtr& packet : sessionPtr->m_sendOverlapped.m_pendingList)
//	{
//		wsaBufs.push_back({ static_cast<ULONG>(packet->GetDataSize()), packet->GetFrontPtr() });
//		//wsaBufs[idx].buf = packet->GetFrontPtr();
//		//wsaBufs[idx].len = packet->GetDataSize();
//		//++idx;
//	}
//
//	InterlockedIncrement(&sessionPtr->ioCount);
//
//	int sendRet = WSASend(sessionPtr->m_socket, wsaBufs.data(), popCount, nullptr, 0, reinterpret_cast<LPWSAOVERLAPPED>(&sessionPtr->m_sendOverlapped), nullptr);
//
//	if (sendRet == SOCKET_ERROR)
//	{
//		DWORD wsaErr = WSAGetLastError();
//
//		// 등록에 실패한 상황
//		if (wsaErr != WSA_IO_PENDING)
//		{
//			//session->m_sendOverlapped.ownerSession = nullptr;
//
//			switch (wsaErr)
//			{
//			case 10053:;
//
//				// 사용자가 일방적으로 연결을 끊은 경우는 에러 출력을 하지 않도록 하겠다.  WSAECONNRESET
//			case 10054:;
//				break;
//			default:
//				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"  SEND POST FAILED .. WSAERROR : %u, 0x%08x", wsaErr, sessionPtr->m_ullSessionId);
//				break;
//			}
//
//			//DecreaseIoCount(sessionPtr, SessionJob::SEND);
//			DecreaseIoCount(sessionPtr);
//
//			return;
//		}
//		InterlockedIncrement(&m_lAsyncSendCount);
//	}
//	InterlockedIncrement(&m_lTotalSendCount);
//}
//
//void jh_network::IocpClient::PostRecv()
//{
//	//if (1 != InterlockedAnd8(&sessionPtr->useFlag, 1))
//	//{
//	//	return;
//	//}
//
//	WSABUF buf[2];
//
//	int directEnqueueSize = sessionPtr->m_recvBuffer.DirectEnqueueSize();
//	int remainderSize = sessionPtr->m_recvBuffer.GetFreeSize() - directEnqueueSize;
//	int wsabufSize = 1;
//
//	buf[0].buf = sessionPtr->m_recvBuffer.GetFrontBufferPtr();
//	buf[0].len = directEnqueueSize;
//
//	if (remainderSize > 0)
//	{
//		++wsabufSize;
//		buf[1].buf = sessionPtr->m_recvBuffer.GetStartBufferPtr();
//		buf[1].len = remainderSize;
//	}
//
//	DWORD flag = 0;
//
//	//if (false == TryIncreaseIoCount(sessionPtr))
//	//	return;
//
//	InterlockedIncrement(&sessionPtr->ioCount);
//
//	int recvRet = WSARecv(sessionPtr->m_socket, buf, wsabufSize, nullptr, &flag, reinterpret_cast<LPWSAOVERLAPPED>(&sessionPtr->m_recvOverlapped), nullptr);
//
//	if (recvRet == SOCKET_ERROR)
//	{
//		int wsaErr = WSAGetLastError();
//
//		if (wsaErr != WSA_IO_PENDING)
//		{
//			switch (wsaErr)
//			{
//				//
//			case 10053:
//				// 사용자가 일방적으로 연결을 끊은 경우는 에러 출력을 하지 않도록 하겠다. WSAECONNRESET
//			case 10054:
//				break;
//			default:
//				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"RECV POST FAILED .. WSAERROR : %u, 0x%08x", wsaErr, sessionPtr->m_ullSessionId);
//				break;
//			}
//
//			//DecreaseIoCount(sessionPtr, SessionJob::RECV);
//			DecreaseIoCount(sessionPtr);
//
//			return;
//		}
//
//		InterlockedIncrement(&m_lAsyncRecvCount);
//	}
//	InterlockedIncrement(&m_lTotalRecvCount);
//}
//
//ErrorCode jh_network::IocpClient::ProcessRecv(DWORD transferredBytes)
//{
//	_clientSession->m_recvOverlapped.ownerSession = nullptr;
//
//	if (transferredBytes == 0)
//	{
//		Disconnect(L"TransferredBytes 0");
//
//		return ErrorCode::NONE; // 정상 종료로 판단.
//	}
//
//	if (false == _clientSession->m_recvBuffer.MoveRearRetBool(transferredBytes))
//	{
//		Disconnect(L"ProcessRecv - Recv Buffer Overflow", true);
//
//		return ErrorCode::RECV_BUF_OVERFLOW;
//	}
//
//	jh_utility::SerializationBuffer m_pPacket(3000);
//
//	while (1)
//	{
//		m_pPacket.Clear();
//
//		int bufferSize = _clientSession->m_recvBuffer.GetUseSize();
//
//		// packetheader보다 작은 상태
//
//
//
//#ifdef ECHO
//		if (bufferSize < sizeof(PacketHeader::size))
//			break;
//#else
//		if (bufferSize < sizeof(PacketHeader))
//			break;
//#endif
//
//		PacketHeader header;
//
//		_clientSession->m_recvBuffer.PeekRetBool(reinterpret_cast<char*>(&header), sizeof(PacketHeader));
//
//#ifdef ECHO
//		if (bufferSize < (sizeof(PacketHeader::size) + header.size))
//			break;
//#else
//		if (bufferSize < (sizeof(PacketHeader) + header.size))
//			break;
//#endif
//
//#ifdef ECHO
//		_recvBuffer.MoveFront(sizeof(header.size));
//#else
//		_clientSession->m_recvBuffer.MoveFront(sizeof(header));
//#endif
//
//		if (false == _clientSession->m_recvBuffer.DequeueRetBool(m_pPacket.GetRearPtr(), header.size))
//		{
//			Disconnect(L"ProcessRecv - Recv Buffer Deque Failed", true);
//
//			return ErrorCode::RECV_BUF_DEQUE_FAILED;
//		}
//
//		m_pPacket.MoveRearPos(header.size);
//
//#ifdef ECHO
//		OnRecv(m_pPacket, 0);
//#else
//		OnRecv(m_pPacket, header.type);
//#endif
//	}
//
//	PostRecv();
//
//	return ErrorCode::NONE;
//}
//
//ErrorCode jh_network::IocpClient::ProcessSend(DWORD transferredBytes)
//{
//	_clientSession->m_sendOverlapped.ownerSession = nullptr;
//	_clientSession->m_sendOverlapped._pendingBuffs.clear();
//
//	InterlockedExchange8(&_clientSession->m_bSendFlag, 0);
//
//	if (transferredBytes == 0)
//	{
//		Disconnect(L"Send Transferred Bytes is 0", true);
//
//		return ErrorCode::SEND_LEN_ZERO;
//	}
//
//	PostSend();
//
//	return ErrorCode::NONE;
//}
