#pragma once

#include <functional>
// ----------------------
//			Job
// ----------------------
namespace jh_utility
{
	// 네트워크로 들어온 packet에 대해서
	// 어떤 쓰레드가 어떤 작업을 해야하는지, 그 작업에 대한 데이터가 들어있는 버퍼.
	struct Job
	{
		explicit Job(ULONGLONG id, USHORT type, PacketPtr packet) :
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

	struct SessionConnectionEvent
	{
		explicit SessionConnectionEvent(ULONGLONG id, jh_utility::SessionConnectionEventType msg) : m_ullSessionId(id), m_msgType(msg) {}
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
}









//namespace jh_utility
//{
//	using CallbackFunc = std::function<void()>;
//	class Job
//	{
//	public:
//
//		Job(CallbackFunc&& callback) : _callback(std::move(callback)) {}
//
//		template <typename T, typename Ret, typename... Args>
//		Job(std::shared_ptr<T> owner, Ret(T::* memFunc)(Args...), Args&&... args)
//		{
//			_callback = [owner, memFunc, args...]()
//			{
//				(owner.get()->*memFunc)(args...);
//			};
//		}
//
//		void Execute()
//		{
//			_callback();
//		}
//	private:
//		CallbackFunc _callback;
//	};
//}