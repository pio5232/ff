#include "LibraryPch.h"
#include "Session.h"
#include "NetworkBase.h"

using namespace jh_utility;

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
	m_connectOverlapped.Reset();

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



jh_network::ActiveSessionManager::ActiveSessionManager(int reserveSize)
{
	InitializeSRWLock(&m_lock);
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

void jh_network::ActiveSessionManager::ProcessAllSession(const std::function<void(ULONGLONG)>& func)
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
