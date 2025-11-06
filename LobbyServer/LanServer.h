#pragma once
#include "NetworkBase.h"
#include "NetworkUtils.h"

namespace jh_content
{
	class LobbyLanSystem;
	class LobbyLanServer : public jh_network::IocpServer
	{
	public:
		LobbyLanServer();
		~LobbyLanServer();

		virtual bool OnConnectionRequest(const SOCKADDR_IN& clientInfo);
		virtual void OnError(int errCode, WCHAR* cause);

		void OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type) override;
		void OnConnected(ULONGLONG sessionId) override;
		void OnDisconnected(ULONGLONG sessionId) override;

		void Init();
		void EndAction() override;

		void SetLobbySystem(class jh_content::LobbySystem* lobbySystem);
	private:
		std::unique_ptr < jh_content::LobbyLanSystem> m_pLobbyLanSystem;
	};
}