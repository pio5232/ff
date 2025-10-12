#pragma once

namespace jh_content
{
	class EchoServer : public jh_network::IocpServer
	{
	public:
		EchoServer();
		~EchoServer();

		void Monitor();
	public:
		virtual void OnRecv(ULONGLONG sessionId, PacketPtr packet,USHORT type) override;
		void OnConnected(ULONGLONG sessionId) override;
		void OnDisconnected(ULONGLONG sessionId) override;

	private:
		void BeginAction() override;
		void EndAction() override;

	private:
		std::unique_ptr<jh_content::EchoSystem> m_pEchoSystem;

		// CIocpServer을(를) 통해 상속됨
	};
}

