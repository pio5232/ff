#pragma once

namespace jh_content
{
	class LobbyDummyClient : public jh_network::IocpClient
	{
	public:
		LobbyDummyClient();
		virtual ~LobbyDummyClient();

		// IocpClient��(��) ���� ��ӵ�
		void OnRecv(ULONGLONG sessionId, PacketPtr dataBuffer, USHORT type) override;

		void OnConnected(ULONGLONG sessionId) override;



	};
}

