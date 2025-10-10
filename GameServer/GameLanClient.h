#pragma once

namespace jh_content
{
	class GameSystem;

	class GameLanClient : public jh_network::IocpClient
	{
	public:
		GameLanClient();
		~GameLanClient();

		void SetGameSystem(class GameSystem* gameSystem) { m_pGameSystem = gameSystem; }
		
		virtual void OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type) override;

		virtual void OnConnected(ULONGLONG sessionId) override;
		virtual void OnDisconnected(ULONGLONG sessionId) override;

	private:
		class GameSystem* m_pGameSystem;
	};
}
