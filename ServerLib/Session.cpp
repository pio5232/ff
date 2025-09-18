#include "LibraryPch.h"
#include "Session.h"
#include "NetworkBase.h"

using namespace jh_utility;

//jh_network::IocpEvent::IocpEvent(IocpEventType type) : iocpEventType(type), ownerSession(nullptr) {}

/*------------------------------
			Session
------------------------------*/

// new로 했을 때 사용.
jh_network::Session::Session()
{
	Reset();
}

jh_network::Session::~Session()
{
}

// 반납, CloseSocket 후 Reset하도록 한다.

void jh_network::Session::Reset()
{
	m_ullSessionId = INVALID_SESSION_ID;

	m_socket = INVALID_SOCKET;
	m_targetNetAddr.Reset();

	m_ullLastTimeStamp = 0; // jh_utility::GetTimeStamp();

	m_sendQ.Clear();
	m_recvBuffer.ClearBuffer();

	m_recvOverlapped.Reset();
	m_sendOverlapped.Reset();
	
	InterlockedExchange8(&m_bSendFlag, 1);
	InterlockedExchange(&m_lIoCount, SESSION_DELETE_FLAG);
	InterlockedExchange8(&m_bConnectedFlag, 0);
}

void jh_network::Session::Activate(SOCKET sock, const SOCKADDR_IN* sockAddr, ULONGLONG newId)
{
	// 세션을 할당할때 사용가능한 상태로 초기화

	m_socket = sock;
	m_targetNetAddr = *sockAddr;
	m_ullLastTimeStamp = jh_utility::GetTimeStamp();

	m_ullSessionId = newId;

	InterlockedExchange8(&m_bSendFlag, 0);
	InterlockedExchange(&m_lIoCount, 0);
	InterlockedExchange8(&m_bConnectedFlag, 1);
}

//void jh_network::Session::Init(SOCKET m_socket, const SOCKADDR_IN* pSockAddr)
//{
//	//LONG isDisconn = InterlockedExchange(&_isDisconnected, 0);
//	//// 할당받았는데 사용중이라고 뜸.
//	//if (0 != isDisconn)
//	//{
//	//	_LOG(L"Session", LOG_LEVEL_WARNING, L"Session Init -> IsDisconnected Flag is Not 0, %ld", isDisconn);
//	//	jh_utility::CrashDump::Crash();
//	//}
//	//this->m_socket = m_socket;
//	//m_targetNetAddr = *pSockAddr;
//	//m_ullLastTimeStamp = GetTimeStamp();
//}




// -----------------------------------------------------------------------------------------------------------------------
//SessionPtr jh_network::SessionManager::CreateSession(SOCKET m_socket, SOCKADDR_IN* pSockAddr, HANDLE iocpHandle)
//{
//	if (_sessionCnt == m_dwMaxSessionCnt)
//	{
//		_LOG(ownerSession->GetServerName(), LOG_LEVEL_INFO, L"CreateSession - Session Count Max");
//
//		return nullptr;
//	}
//	_sessionCnt.fetch_add(1);
//
//	SessionPtr newSession = _createFunc();
//
//	newSession->Init(m_socket, pSockAddr);
//
//	if (nullptr == CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_socket), iocpHandle, 0, 0))
//	{
//		_sessionCnt.fetch_sub(1);
//
//		_LOG(ownerSession->GetServerName(), LOG_LEVEL_INFO, L"CreateSession - Session Register Failed");
//
//		return nullptr;
//	}
//		
//	{
//		SRWLockGuard lockGuard(&m_lock);
//		
//		_sessionSet.insert(newSession);
//	}
//
//
//	return newSession;
//} 
//
//void jh_network::SessionManager::DeleteSession(SessionPtr sessionPtr)
//{
//	SRWLockGuard lockGuard(&m_lock);
//	
//	_sessionSet.erase(sessionPtr);
//	
//	_sessionCnt.fetch_sub(1);
//}
//
////
////void jh_network::SessionManager::CheckHeartbeatTimeOut(ULONGLONG now)
////{
////	int sessionCnt = _sessionCnt;
////
////	if (sessionCnt == 0)
////		return;
////
////	std::vector<SessionPtr> tempSessions;
////	tempSessions.reserve(sessionCnt);
////	{
////		SRWLockGuard lockGuard(&m_lock);
////
////		for (SessionPtr sessionPtr : _sessionSet)
////		{
////			tempSessions.push_back(sessionPtr);
////		}
////	}
////
////	for (SessionPtr& sessionPtr : tempSessions)
////	{
////		sessionPtr->CheckHeartbeatTimeout(now);
////	}
////}
//
//jh_network::SessionManager::~SessionManager()
//{
//	SRWLockGuard lockGuard(&m_lock);
//	
//	_sessionSet.clear();
//}
//

jh_network::ActiveSessionManager::ActiveSessionManager(int reserveSize)
{
}

jh_network::ActiveSessionManager::~ActiveSessionManager()
{
}

void jh_network::ActiveSessionManager::AddActiveSession(Session* sessionPtr)
{
	SRWLockGuard lockGuard(&m_lock);
	
	ULONGLONG sessionIdWithIdx = sessionPtr->m_ullSessionId;
	
	m_activeSessionMap.insert(std::make_pair(sessionIdWithIdx, sessionPtr));
}

void jh_network::ActiveSessionManager::RemoveActiveSession(ULONGLONG sessionIdWithIdx)
{
	SRWLockGuard lockGuard(&m_lock);

	m_activeSessionMap.erase(sessionIdWithIdx);
}

void jh_network::ActiveSessionManager::ProcessAllSessions(const std::function<void(ULONGLONG)>& func)
{
	std::vector<ULONGLONG> delSessions;
	delSessions.reserve(5);

	delSessions.clear();

	{
		SRWLockGuard lockGuard(&m_lock);

		for (const auto [sessionIdWithIdx, sessionPtr] : m_activeSessionMap)
		{
			if (sessionIdWithIdx != sessionPtr->m_ullSessionId)
				delSessions.push_back(sessionIdWithIdx);

			func(sessionIdWithIdx);
		}
	}

	for (ULONGLONG idWithIdx : delSessions)
	{
		RemoveActiveSession(idWithIdx);
	}
}
