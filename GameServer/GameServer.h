#pragma once
#include <atomic>
#include "Utils.h"

namespace jh_content
{
	class GameSystem;
	class GameLanClient;

	class GameWorld;
	class UserManager;

	class GameServer : public jh_network::IocpServer
	{
	public:
		
		GameServer();

		~GameServer();

		virtual bool OnConnectionRequest(const SOCKADDR_IN& clientInfo);
		virtual void OnError(int errCode, WCHAR* cause);

		void OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type) override;
		void OnConnected(ULONGLONG sessionId) override;
		void OnDisconnected(ULONGLONG sessionId) override;

		void BeginAction() override;
		void EndAction() override;


		void Monitor();
	private:
		std::unique_ptr<class jh_content::GameSystem> m_pGameSystem;
		std::unique_ptr<class jh_content::GameLanClient> m_pGameLanClient;
	};

}