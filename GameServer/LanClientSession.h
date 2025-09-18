#pragma once
namespace jh_network
{
	class LanClientSession : public jh_network::Session
	{
	public:
		LanClientSession(std::weak_ptr<ServerBase> gameServer);

		virtual void OnConnected() override;
		virtual void OnDisconnected() override;
		virtual void OnRecv(jh_utility::SerializationBuffer& buffer, uint16 type) override;

		std::shared_ptr<class GameServer> GetGameServer();
	private:
		std::weak_ptr<class GameServer> _gameServer;
	};
}