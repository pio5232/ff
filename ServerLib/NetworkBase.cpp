#include "LibraryPch.h"
#include <MSWSock.h>
#include <Windows.h>
#include <conio.h>
#include "NetworkBase.h"

#include "ws2tcpip.h"
#include "Session.h"
#include "Memory.h"
#include "Job.h"

//#define ECHO

#pragma comment (lib, "ws2_32.lib")
	/*-----------------------
		  IocpServer
	-----------------------*/

jh_network::IocpServer::IocpServer(const WCHAR* serverName) : m_hCompletionPort(nullptr), m_listenSock(INVALID_SOCKET), m_pcwszServerName(serverName), m_pSessionArray(nullptr), m_wszIp{},
m_hWorkerThreads{}, m_hAcceptThread{}, m_hDispatchThread{}, m_hStopEvent{}, m_hSendEvent{}
{
	m_dwConcurrentWorkerThreadCount = 0;
	m_lingerOption = {};
	m_dwMaxSessionCnt = 0;
	m_usPort = 0;
	m_ullTimeOutLimit = 0;

	//_sessionLog = new SessionLog[sessionLogMax];

	m_lSessionCount = 0;
	m_llTotalAcceptedSessionCount = 0;

	m_llDisconnectedCount = 0;
	m_llTotalDisconnectedCount = 0;
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
	if (0 == m_dwConcurrentWorkerThreadCount)
	{
		SYSTEM_INFO sys;
		GetSystemInfo(&sys);

		m_dwConcurrentWorkerThreadCount = sys.dwNumberOfProcessors;
	}

	_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Concurrent ThreadCount : %u", m_dwConcurrentWorkerThreadCount);

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, m_dwConcurrentWorkerThreadCount);

	if (NULL == m_hCompletionPort)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"IOCP Handle is Null");

		return false;
	}

	m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_listenSock)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"ListenSocket is Invalid");

		return false;
	}

	SOCKADDR_IN sockaddrIn{AF_INET, htons(m_usPort), NetAddress::IpToAddr(m_wszIp) };
	//sockaddrIn.sin_family = AF_INET;
	//sockaddrIn.sin_port = htons(m_usPort);
	//sockaddrIn.sin_addr = NetAddress::IpToAddr(m_wszIp);

	DWORD bindRet = bind(m_listenSock, (SOCKADDR*)&sockaddrIn, sizeof(SOCKADDR_IN));

	if (SOCKET_ERROR == bindRet)
	{
		int getLastError = WSAGetLastError();

		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Bind is Failed. GetLastError : %d",getLastError);

		return false;
	}

	int sndBuffSize = 0;
	DWORD zeroCpyRet = setsockopt(m_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBuffSize, sizeof(sndBuffSize));
	if (SOCKET_ERROR == zeroCpyRet)
	{
		int getLastError = WSAGetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Set ZeroCpy Failed. GetLastError : %d", getLastError);

		return false;

	}
	DWORD setLingerRet = setsockopt(m_listenSock, SOL_SOCKET, SO_LINGER, (char*)&m_lingerOption, sizeof(linger));
	if (SOCKET_ERROR == setLingerRet)
	{
		int getLastError = WSAGetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"Set Linger Failed. GetLastError : %d",getLastError);
		
		return false;
	}

	if (false == CreateServerThreads())
		return false;

	// AUTO, NON-SIGNALED
	m_hStopEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == m_hStopEvent)
	{
		int getLastError = WSAGetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"StopEvent Creation is Failed. GetLastError : %d", getLastError);

		return false;
	}

	m_hSendEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == m_hSendEvent)
	{
		int getLastError = WSAGetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"SendEvent Creation is Failed. GetLastError : %d", getLastError);

		return false;
	}

	BeginAction();

	return true;
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

void jh_network::IocpServer::Stop()
{
	EndAction();
	
	SetEvent(m_hStopEvent);

	{
		DWORD waitSingleRet = WaitForSingleObject(m_hDispatchThread, INFINITE);
		if (waitSingleRet != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();
			_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"[Stop] - m_hDispatchThread Wait return Error : [%u]", gle);
		}
	}

	closesocket(m_listenSock);
	m_listenSock = INVALID_SOCKET;
	
	{
		DWORD waitSingleRet = WaitForSingleObject(m_hAcceptThread, INFINITE);
		if (waitSingleRet != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();
			_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"[Stop] - m_hAccept Wait return Error : [%u]", gle);
		}
	}

	for (int i = 0; i < m_dwConcurrentWorkerThreadCount; i++)
	{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, nullptr);
	}

	{
		DWORD waitMultipleRet = WaitForMultipleObjects(m_dwConcurrentWorkerThreadCount, m_hWorkerThreads, true, INFINITE);

		if (waitMultipleRet != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();
			_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"[Stop] - m_hWorkerThreads Wait return Error : [%u]", gle);
		}

		for (int i = 0; i < m_dwConcurrentWorkerThreadCount; i++)
		{
			CloseHandle(m_hWorkerThreads[i]);
			m_hWorkerThreads[i] = nullptr;
		}
	}

	CloseHandle(m_hStopEvent);
	CloseHandle(m_hSendEvent);
	CloseHandle(m_hDispatchThread);
	CloseHandle(m_hAcceptThread);

	m_hSendEvent = nullptr;
	m_hStopEvent = nullptr;
	m_hDispatchThread = nullptr;
	m_hAcceptThread = nullptr;


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
	// 정리되지 않은 세션들을 정리
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


bool jh_network::IocpServer::CreateServerThreads()
{
	m_hWorkerThreads = static_cast<HANDLE*>(g_memAllocator->Alloc(sizeof(HANDLE) * m_dwConcurrentWorkerThreadCount));

	for (int i = 0; i < m_dwConcurrentWorkerThreadCount; i++)
	{
		m_hWorkerThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, WorkerThreadFunc, this, 0, nullptr));

		if (nullptr == m_hWorkerThreads[i])
		{
			DWORD gle = GetLastError();
			_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"WorkerThread Creation is Failed, Error Code : [%u]", gle);

			g_memAllocator->Free(m_hWorkerThreads);
			return false;
		}
	}

	// TODO_
	m_hAcceptThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, AcceptThreadFunc, this, 0, nullptr));
	if (nullptr == m_hAcceptThread)
	{
		DWORD gle = GetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"AcceptThread Creation is Failed, Error Code : [%u]", gle);

		return false;
	}

	m_hDispatchThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, DispatchThreadFunc, this, 0, nullptr));
	if (nullptr == m_hDispatchThread)
	{
		DWORD gle = GetLastError();
		_LOG(m_pcwszServerName, LOG_LEVEL_SYSTEM, L"DispatchThread Creation is Failed, Error Code : [%u]", gle);

		return false;
	}

	return true;
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

	if (InterlockedCompareExchange(&sessionPtr->m_lIoCount, SESSION_DELETE_FLAG, 0) != 0)
	{
		_LOG(m_pcwszServerName, LOG_LEVEL_INFO, L"Delete SessION -> session is Using");
		return;
	}

	OnDisconnected(sessionId);

	m_activeSessionManager.RemoveActiveSession(sessionId);

	closesocket(sessionPtr->m_socket);

	sessionPtr->Reset();

	m_sessionIndexStack.Push(sessionIdx);
	
	InterlockedIncrement64(&m_llTotalDisconnectedCount);

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

unsigned __stdcall jh_network::IocpServer::WorkerThreadFunc(LPVOID lparam)
{
	IocpServer* instance = reinterpret_cast<IocpServer*>(lparam);

	instance->WorkerThreadMain();

	return 0;
}

unsigned __stdcall jh_network::IocpServer::AcceptThreadFunc(LPVOID lparam)
{
	IocpServer* instance = reinterpret_cast<IocpServer*>(lparam);

	instance->AcceptThreadMain();

	return 0;
}

unsigned __stdcall jh_network::IocpServer::DispatchThreadFunc(LPVOID lparam)
{
	IocpServer* instance = reinterpret_cast<IocpServer*>(lparam);

	instance->DispatchThreadMain();

	return 0;
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

void jh_network::IocpServer::WorkerThreadMain()
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
			DWORD er = WSAGetLastError();

			_LOG(L"Server GQCS_FALSE", LOG_LEVEL_WARNING, L"WSAGetLastError : %u", er);

			InterlockedIncrement64(&m_llDisconnectedCount);
			
			Disconnect(sessionPtr->m_ullSessionId, L"gqcsRet False");

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

	if (false == sessionPtr->m_recvBuffer.MoveRear(transferredBytes))
	{
		Disconnect(sessionPtr->m_ullSessionId, L"ProcessRecv - Recv Buffer Overflow");

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

		//jh_utility::SerializationBuffer* packet = static_cast<jh_utility::SerializationBuffer*>(g_memAllocator->Alloc(header));
		PacketPtr packet = MakeSharedBuffer(g_memAllocator, header);


		//if (false == sessionPtr->m_recvBuffer.DequeueRetBool(pPacket->GetRearPtr(), header.size))
		if (false == sessionPtr->m_recvBuffer.DequeueRetBool(packet->GetRearPtr(), header))
		{
			Disconnect(sessionPtr->m_ullSessionId, L"ProcessRecv - Recv Buffer Deque Failed");

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
			Disconnect(sessionPtr->m_ullSessionId, L"ProcessRecv - Recv Buffer Deque Failed");

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

void jh_network::IocpServer::Disconnect(ULONGLONG sessionId, const WCHAR* reason)
{
	Session* sessionPtr = TryAcquireSession(sessionId, PROF_WFUNC);

	if (nullptr == sessionPtr)
		return;

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

	static thread_local alignas(64) std::queue<PacketPtr> tempQ;

	sessionPtr->m_sendQ.Swap(tempQ);

	int popCount = tempQ.size();
	
	while (tempQ.size() > 0)
	{
		sessionPtr->m_sendOverlapped.m_pendingList.push_back(std::move(tempQ.front()));
		
		tempQ.pop();
	}

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
			default:
				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L" [PostSend]	에러 코드 : %u,  세션 ID : [0x%08x]", wsaErr, sessionPtr->m_ullSessionId);
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
	//if (1 != InterlockedAnd8(&sessionPtr->m_bUseFlag, 1))
	//{
	//	return;
	//}

	if (InterlockedAnd8(&sessionPtr->m_bConnectedFlag, 1) == 0)
		return;

	WSABUF buf[2]{};

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
			default:
				_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L" [PostRecv]	에러 코드 : %u,  세션 ID : [0x%08x]", wsaErr, sessionPtr->m_ullSessionId);
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

void jh_network::IocpServer::RequestSend(ULONGLONG sessionId, PacketPtr& packet)
{
	jh_utility::DispatchJob* dispatchJob = jh_memory::ObjectPool<jh_utility::DispatchJob>::Alloc(sessionId, packet);

	RequestSend(dispatchJob);
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
	static alignas(64) std::vector<ULONGLONG> disconnList;
	std::vector<ULONGLONG> emptyVec;

	auto func = [this, now](ULONGLONG sessionId) {
		Session* sessionPtr = TryAcquireSession(sessionId, PROF_WFUNC);

		if (nullptr == sessionPtr)
		{
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"CheckHeartbeatTimeout - TryAcquireSession Failed, %0x", sessionId);
			return;
		}

		if (now - sessionPtr->m_ullLastTimeStamp > m_ullTimeOutLimit)
		{
			disconnList.push_back(sessionId);
		}
		DecreaseIoCount(sessionPtr);
	};

	m_activeSessionManager.ProcessAllSessions(func);

	for (ULONGLONG sessionId : disconnList)
	{
		Disconnect(sessionId, L"Heartbeat");
	}
	
	disconnList.swap(emptyVec);
}

// 0이 리턴되는 경우는 잘못된 경우
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



void jh_network::IocpServer::AcceptThreadMain()
{
	while (1)
	{
		SOCKADDR_IN clientInfo{};
		int infoSize = sizeof(clientInfo);
		SOCKET clientSock = accept(m_listenSock, (SOCKADDR*)&clientInfo, &infoSize);

		// Accept 종료
		if (clientSock == INVALID_SOCKET)
		{
			DWORD lastErr = WSAGetLastError();
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
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"[AcceptThreadMain] 세션이 nullptr입니다.");

			closesocket(clientSock);
			continue;
		}
		
		OnConnected(newSession->m_ullSessionId);

		PostRecv(newSession);
	}
}

void jh_network::IocpServer::DispatchThreadMain()
{
	HANDLE handles[2]{ m_hStopEvent, m_hSendEvent };

	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, handles, false, INFINITE);
		
		// StopEvent
		if (WAIT_OBJECT_0 == ret)
		{
			break;
		}
		// SendEvent
		if (WAIT_OBJECT_0 + 1 == ret)
		{
			DispatchPacket();
		}
		else
		{
			DWORD gle = GetLastError();
			_LOG(m_pcwszServerName, LOG_LEVEL_WARNING, L"[DispatchThreadMain] WaitMultiple Ret Error : [%u].",gle);
		}
	}
}


void jh_network::IocpServer::DispatchPacket()
{
	static thread_local std::queue<jh_utility::DispatchJob*> dispatchQ;

	m_dispatchJobQ.Swap(dispatchQ);

	while (dispatchQ.size())
	{
		jh_utility::DispatchJob* dispatchJob = dispatchQ.front();

		dispatchQ.pop();

		if (1 == dispatchJob->m_dwSessionCount)
		{
			SendPacket(dispatchJob->m_sessionInfo.m_ullSingleSessionId, dispatchJob->m_packet);
		}
		else
		{
			for (int i = 0; i < dispatchJob->m_dwSessionCount; i++)
			{
				SendPacket(dispatchJob->m_sessionInfo.m_pSessionIdList[i], dispatchJob->m_packet);
			}
		}

		jh_memory::ObjectPool<jh_utility::DispatchJob>::Free(dispatchJob);
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
//	DWORD m_dwThreadId = GetCurrentThreadId();
//	LONG order = InterlockedIncrement(&logIndex) % sessionLogMax;
//
//	_sessionLog[order].m_dwThreadId = m_dwThreadId;
//	_sessionLog[order].m_llSessionId = m_llSessionId;
//	_sessionLog[order].sessionIoCount = m_lIoCount;
//	_sessionLog[order].sessionJob = sessionJob;
//
//}

// ----------------- IocpClient ----------------------

void jh_network::IocpClient::ProcessRecv(Session* sessionPtr, DWORD transferredBytes)
{
	if (false == sessionPtr->m_recvBuffer.MoveRear(transferredBytes))
	{
		Disconnect(sessionPtr->m_ullSessionId, L"recvBuffer MoveRear Failed");

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
			Disconnect(sessionPtr->m_ullSessionId, L"Echo Dequeue Failed");

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
			Disconnect(sessionPtr->m_ullSessionId, L"Dequeue Failed");

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

void jh_network::IocpClient::Disconnect(ULONGLONG sessionId, const WCHAR* reason)
{
	Session* sessionPtr = TryAcquireSession(sessionId);

	if (nullptr == sessionPtr)
		return;

	_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"Enter Disconnect Log - %s", reason);

	// 연결 끊긴 상태가 아닌, 먼저 연결을 끊는 상황에서
	// 모두 완료 통지가 들어왔을 때 Disconnect()가 호출된다면
	// 그다음 완료 통지를 처리할 때 i/o 등록을 막는다.
	InterlockedExchange8(&sessionPtr->m_bConnectedFlag, 0);

	// recv, send가 등록되었다면 둘 다 io 취소한다.
	CancelIoEx(reinterpret_cast<HANDLE>(sessionPtr->m_socket), nullptr);

	DecreaseIoCount(sessionPtr);
}

void jh_network::IocpClient::PostSend(Session* sessionPtr)
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

	static thread_local alignas(64) std::queue<PacketPtr> tempQ;

	sessionPtr->m_sendQ.Swap(tempQ);

	size_t popCount = tempQ.size();

	while (tempQ.size() > 0)
	{
		sessionPtr->m_sendOverlapped.m_pendingList.push_back(std::move(tempQ.front()));

		tempQ.pop();
	}

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

		// 등록에 실패한 상황
		if (wsaErr != WSA_IO_PENDING)
		{
			_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"  [SendPost] Failed. WsaError : [%u], SessionID : [0x%016x]", wsaErr, sessionPtr->m_ullSessionId);

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

	WSABUF buf[2]{};

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
			_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[RecvPost] Failed. WsaError : [%u], SessionID : [0x%016x]", wsaErr, sessionPtr->m_ullSessionId);

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

	instance->WorkerThreadMain();

	return 0;
}

void jh_network::IocpClient::WorkerThreadMain()
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

				_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"ProcessIO  Close Completion port [WSAERROR : %d]", wsaErr);
			}
			break;
		}

		// lpOverlapped != nullptr, gqcsRet == false
		// 작업 도중 연결이 끊겼을 때의 상황이다.
		if (false == gqcsRet)
		{			
			auto er = WSAGetLastError();
			
			_LOG(L"Client GQCS_FALSE", LOG_LEVEL_WARNING, L"WSAGetLastError : %u", er);
			InterlockedIncrement(&m_llDisconnectedCount);

			Disconnect(sessionPtr->m_ullSessionId, L"gqcs false");

			DecreaseIoCount(sessionPtr);

			continue;
		}

		if (&sessionPtr->m_connectOverlapped == lpOverlapped)
		{
			// api사용을 위해 소켓 정보 초기화
			setsockopt(sessionPtr->m_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);

			OnConnected(sessionPtr->m_ullSessionId);
		
			InterlockedIncrement64(&m_llTotalConnectedSessionCount);

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
		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[TryAcquireSession] - Invalid Session. Session ID [0x%0x]", sessionId);
		return nullptr;
	}

	LONGLONG sessionIdx = sessionId >> SESSION_IDX_SHIFT_BIT;

	Session* sessionPtr = &m_pClientSessionArr[sessionIdx];
	// 세션을 그냥 얻어가면 줬을 때 내가 준 세션이 해제되었을 수 있다. 그래서 ID 확인 후 건네주도록 한다.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		// 이미 해제된 세션에 접근하는 것은 아주 정상적인 상태.
		_LOG(m_pcwszClientName, LOG_LEVEL_DEBUG, L"[TryAcquireSession] - Accessed SessionID is Different. SessionID : [%lld], SessionIdx : [%lld]", sessionId, sessionIdx);

		return nullptr;
	}

	// 받은 후에도. 받은 세션에 대해서 사용하기 전에 이 녀석이 정리중인가 아닌가를 확인하는 과정이 필요하다.
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
	// 여기부터 세션빼서 내껄로 사용하는거 적기,
	DWORD availableIndex = UINT_MAX;

	if (false == m_sessionIndexStack.TryPop(availableIndex))
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_INFO, L"[CreateSession] Available Session Count is 0. Current Session Count : [%u]", m_lSessionCount);

		return nullptr;
	}

	// 세션 수를 증가하고.
	InterlockedIncrement(&m_lSessionCount);

	Session* sessionPtr = &m_pClientSessionArr[availableIndex];

	// 세션 초기화
	static ULONGLONG sessionIdGen = 0;

	//sessionPtr->m_socket = sock;
	//sessionPtr->m_targetNetAddr = *pSockAddr;
	//sessionPtr->m_ullLastTimeStamp = jh_utility::GetTimeStamp();

	// Session에 등록할 id
	// [16 idx ][48 id]

	ULONGLONG id = (static_cast<ULONGLONG>(availableIndex) << SESSION_IDX_SHIFT_BIT) | ((++sessionIdGen) & SESSION_ID_MASKING_BIT);

	//sessionPtr->m_ullSessionId = id;

	sessionPtr->Activate(sock, pSockAddr, id);

	_LOG(L"Session", LOG_LEVEL_DETAIL, L"CreateSession -  Session Index : 0x%08x, Session ID : 0x%08x ", availableIndex, id);

	// 세션 등록 실패
	HANDLE ret = CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock), m_hCompletionPort, reinterpret_cast<ULONG_PTR>(sessionPtr), 0);
	if (nullptr == ret)
	{
		m_sessionIndexStack.Push(availableIndex);

		InterlockedDecrement(&m_lSessionCount);

		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"CreateSession - Session Register Failed");

		return nullptr;
	}

	return sessionPtr;

}

void jh_network::IocpClient::DeleteSession(ULONGLONG sessionId)
{
	if (sessionId == INVALID_SESSION_ID)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[DeleteSession] - 유효하지 않은 세션입니다. Session Id : [%llu]",sessionId);
		return;
	}

	LONGLONG sessionIdx = sessionId >> SESSION_IDX_SHIFT_BIT;

	Session* sessionPtr = &m_pClientSessionArr[sessionIdx];
	// 세션을 그냥 얻어가면 줬을 때 내가 준 세션이 해제되었을 수 있다. 그래서 ID 확인 후 건네주도록 한다.
	if (sessionId != sessionPtr->m_ullSessionId)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_WARNING, L"[DeleteSession] - 접근한 세션의 ID가 다릅니다. SessionID : [%lld], SessionIdx : [%lld]", sessionId, sessionIdx);

		return;
	}

	if (InterlockedCompareExchange(&sessionPtr->m_lIoCount, SESSION_DELETE_FLAG, 0) != 0)
	{
		_LOG(m_pcwszClientName, LOG_LEVEL_INFO, L" [DeleteSession] - 사용중인 세션입니다. SessionId : [%llu]", sessionId);
		return;
	}
	InterlockedIncrement64(&m_llTotalDisconnectedCount);

	OnDisconnected(sessionId);

	closesocket(sessionPtr->m_socket);

	sessionPtr->Reset();

	m_sessionIndexStack.Push(sessionIdx);

	InterlockedDecrement(&m_lSessionCount);

	_LOG(m_pcwszClientName, LOG_LEVEL_INFO, L"[DeleteSession] - 세션이 해제되었습니다.. Session ID [0x%0x]", sessionId);

	//_LOG(L"Session", LOG_LEVEL_DETAIL, L"Delete PushStack SessionIdx, SessionID : 0x%08x, SessionIdx : 0x%08x", sessionId, sessionIdx);

	// Delete는 얻어온 세션에 대해서 ioCount를 새로 밀어버리기 때문에 정리하지 않도록 한다..
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

	for (DWORD dw = 0; dw < m_dwMaxSessionCnt; dw++)
	{
		m_sessionIndexStack.Push(dw);
	}

	return true;
}

void jh_network::IocpClient::ForceStop()
{
	// 정리되지 않은 세션들을 정리
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

	m_llDisconnectedCount = 0;
	m_llTotalDisconnectedCount = 0;

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
		_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Start] IOCP Handle이 Null입니다.");

		return false;
	}

	m_hWorkerThreads = new HANDLE[m_dwConcurrentWorkerThreadCount];

	for (int i = 0; i < m_dwConcurrentWorkerThreadCount; i++)
	{
		m_hWorkerThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, IocpClient::WorkerThreadFunc, this, 0, nullptr));

		if (nullptr == m_hWorkerThreads[i])
		{
			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Start] WorkerThread %d 가 존재하지 않습니다.", i);

			jh_utility::CrashDump::Crash();
		}
	}

	BeginAction();

	return true;
}

void jh_network::IocpClient::Stop()
{
	EndAction();

	if (nullptr != m_hWorkerThreads)
	{
		for(int i =0 ;i< m_dwConcurrentWorkerThreadCount;i++)
		{
			PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, nullptr);
		}

		DWORD ret = WaitForMultipleObjects(m_dwConcurrentWorkerThreadCount, m_hWorkerThreads, true, INFINITE);
		if (!(WAIT_OBJECT_0 <= ret && (WAIT_OBJECT_0 + m_dwConcurrentWorkerThreadCount) > ret))
		{
			DWORD gle = GetLastError();
			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Stop] - WaitForMultipleObject return Error : [%u]",gle);
		}

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

void jh_network::IocpClient::Connect(int cnt)
{
	SOCKADDR_IN serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(m_usTargetPort);
	serverAddr.sin_addr = NetAddress::IpToAddr(m_wszTargetIp);

	for (int i = 0; i < cnt; i++)
	{

		SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == sock)
		{
			int getLastError = WSAGetLastError();

			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] 유효하지 않은 소켓입니다. GetLastError : [%d]", getLastError);

			closesocket(sock);
			return;
		}
		DWORD setLingerRet = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&m_lingerOption, sizeof(linger));

		if (SOCKET_ERROR == setLingerRet)
		{
			int getLastError = WSAGetLastError();

			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] SetLingerOpt 실패. GetLastError : [%d]", getLastError);

			closesocket(sock);
			return;
		}
		// connectEx 사용 시 bind 필요
		sockaddr_in clientAddr{};
		clientAddr.sin_family = AF_INET;
		//InetPton(AF_INET,m_wszTargetIp, &clientAddr.sin_addr);
		clientAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		clientAddr.sin_port = htons(0);

		DWORD bindRet = bind(sock, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR_IN));

		if (SOCKET_ERROR == bindRet)
		{
			int getLastError = WSAGetLastError();

			_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] Bind 실패. GetLastError : [%d]", getLastError);

			closesocket(sock);

			return;
		}
		Session* session = CreateSession(sock, &serverAddr);

		if (nullptr == session)
		{
			closesocket(sock);

			return;
		}

		//connect에 대한 ioCount 따로 등록
		InterlockedIncrement(&session->m_lIoCount);

		DWORD bytes;

		bool retConnectEx = jh_network::NetAddress::lpfnConnectEx(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr), nullptr, 0, &bytes, &session->m_connectOverlapped);

		if (false == retConnectEx)
		{
			int wsaGetLastError = WSAGetLastError();

			if (WSA_IO_PENDING != wsaGetLastError)
			{
				_LOG(m_pcwszClientName, LOG_LEVEL_SYSTEM, L"[Connect] ConnectEx 실패. Session ID : [%llu]", session->m_ullSessionId);

				DecreaseIoCount(session);
			}
		}
	}
	//if(jh_network::NetAddress::lpfnConnectEx)
	// TODO_
}
