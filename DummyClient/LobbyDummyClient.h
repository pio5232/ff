#pragma once

namespace jh_content
{
	class DummyUpdateSystem;
	class LobbyDummyClient : public jh_network::IocpClient
	{
	public:
		LobbyDummyClient();
		virtual ~LobbyDummyClient();

		// IocpClient��(��) ���� ��ӵ�
		void OnRecv(ULONGLONG sessionId, PacketPtr dataBuffer, USHORT type) override;

		void OnConnected(ULONGLONG sessionId) override;
		void OnDisconnected(ULONGLONG sessionId) override;

		std::unique_ptr<jh_content::DummyUpdateSystem> m_pDummySystem;

		virtual void BeginAction() override;
		virtual void EndAction() override;

	};
}

