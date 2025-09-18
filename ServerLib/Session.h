#pragma once

#include <queue>
#include <stack>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <set>

#define INVALID_SESSION_ID (LONGLONG)(~0)
#define INVALID_SESSION_INDEX (LONGLONG)(~0)

#define SESSION_IDX_SHIFT_BIT 48
#define SESSION_IDX_SHIFT_MASKING_BIT (0xffff'0000'0000'0000)

#define SESSION_ID_MASKING_BIT (0x0000'ffff'ffff'ffff)
#define SESSION_DELETE_FLAG 0x8000'0000

enum class OverlappedType
{
	RECV,
	SEND,
	CONNECT,
};

struct CustomOverlapped : public OVERLAPPED
{
	CustomOverlapped(OverlappedType overlappedType) : OVERLAPPED{}, m_eOverLappedType { overlappedType }
	{

	}

	const OverlappedType m_eOverLappedType;

};
struct RecvOverlapped : public CustomOverlapped
{
	RecvOverlapped() : CustomOverlapped(OverlappedType::RECV){}
	
	// 가상 함수로 만들면 안됨!!
	void Reset() 
	{
		memset(this, 0, sizeof(OVERLAPPED));
	}
};

struct SendOverlapped : public CustomOverlapped
{
	SendOverlapped() : CustomOverlapped(OverlappedType::SEND)
	{
		//m_pendingList.reserve(20);
	}

	void Reset()
	{
		ClearPendingList();

		memset(this, 0, sizeof(OVERLAPPED));
	}

	void ClearPendingList()
	{
		// 이 For 루프 부분을 RefCount로 사용했을 때 바꿔줘야한다.

		//for (int i = 0; i < m_pendingList.size(); i++)
		//{
		//	g_packetPool->Free(m_pendingList[i]);
		//}
		m_pendingList.clear();
	}

	//std::vector<jh_utility::SerializationBuffer*> m_pendingList;
	std::vector<PacketPtr> m_pendingList;
};

struct ConnectOverlapped : public CustomOverlapped
{
	ConnectOverlapped() : CustomOverlapped(OverlappedType::CONNECT) {}

	void Reset()
	{
		memset(this, 0, sizeof(OVERLAPPED));
	}

};

namespace jh_network
{
	/*------------------------------
				Session 
	------------------------------*/

	// [ 4 (arrayIndex) / 4 (id) ]
	struct Session 
	{
	public:
		Session();
		~Session();

		void Reset();
		void Activate(SOCKET sock, const SOCKADDR_IN* sockAddr, ULONGLONG newId);
		//void Init(SOCKET m_socket, const SOCKADDR_IN* pSockAddr);

	private:
		LONGLONG GetSessionId() { return m_ullSessionId & SESSION_ID_MASKING_BIT; }
		LONGLONG GetSessionIdx() { return m_ullSessionId >> SESSION_IDX_SHIFT_BIT; }
	public:
		LONGLONG m_ullSessionId;
		SOCKET m_socket;
		NetAddress m_targetNetAddr;

		ULONGLONG m_ullLastTimeStamp;
	
		jh_utility::LockQueue<PacketPtr> m_sendQ;
		jh_utility::RingBuffer m_recvBuffer;

		RecvOverlapped m_recvOverlapped;
		SendOverlapped m_sendOverlapped;

		alignas(64) LONG m_lIoCount;
		alignas(64) char m_bSendFlag; // Use - 1, unUse - 0
		alignas(64) char m_bConnectedFlag;
	};

	/*------------------------------------------
				ActiveSessionManager
	------------------------------------------*/

	class ActiveSessionManager
	{
	public:
		ActiveSessionManager(int reserveSize = 10);
		~ActiveSessionManager();

		void AddActiveSession(Session* sessionPtr);
		void RemoveActiveSession(ULONGLONG sessionIdWithIdx); // [index(16) + SessionId]

		void ProcessAllSessions(const std::function<void(ULONGLONG)>& func);
	private:
		std::unordered_map<ULONGLONG, Session*> m_activeSessionMap;
		SRWLOCK m_lock;
	};
}
