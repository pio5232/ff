#pragma once

#include <functional>
// ----------------------
//			Job
// ----------------------
namespace jh_utility
{
	/// <summary>
	/// 서버가 해야할 작업의 용도를 알리는 용도로 사용된다.
	/// </summary>
	struct Job
	{
		Job(ULONGLONG id, USHORT type, PacketPtr packet) :
			m_llSessionId(id), m_wJobType(type), m_pPacket(packet) {}
		ULONGLONG m_llSessionId;
		USHORT m_wJobType; 
		PacketPtr m_pPacket;

		~Job()
		{
			m_llSessionId = INVALID_SESSION_ID;
			m_wJobType = (USHORT)(jh_network::INVALID_PACKET);
			m_pPacket.reset();
		}
		
		Job(const Job& other) = default;
		Job(Job&& other) = default;

		Job& operator=(const Job& other) = default;
		Job& operator=(Job&& other) = default;
	};


	enum class SessionConnectionEventType : byte
	{
		NONE = 0,
		CONNECT = 1,
		DISCONNECT,
	};

	/// <summary>
	/// Session의 연결 / 해제를 알리는 용도로 사용된다.
	/// </summary>
	struct SessionConnectionEvent
	{
		SessionConnectionEvent(ULONGLONG id, jh_utility::SessionConnectionEventType msg) : m_ullSessionId(id), m_msgType(msg) {}
		~SessionConnectionEvent()
		{
			m_ullSessionId = INVALID_SESSION_ID;
			m_msgType = SessionConnectionEventType::NONE;
		}

		SessionConnectionEvent(const SessionConnectionEvent& other) = default;
		SessionConnectionEvent(SessionConnectionEvent&& other) = default;

		SessionConnectionEvent& operator=(const SessionConnectionEvent& other) = default;
		SessionConnectionEvent& operator=(SessionConnectionEvent&& other) = default;

		ULONGLONG m_ullSessionId; 
		SessionConnectionEventType m_msgType;
	};

	struct DispatchJob
	{
		DispatchJob(ULONGLONG sessionId, PacketPtr& packetPtr);
		DispatchJob(DWORD sessionCount, ULONGLONG* sessionIdList, PacketPtr& packetPtr);

		~DispatchJob();
		DWORD m_dwSessionCount{};
		PacketPtr m_packet{};
		union SessionInfo
		{
			ULONGLONG m_ullSingleSessionId;
			ULONGLONG* m_pSessionIdList;
		} m_sessionInfo{};
	};
}





