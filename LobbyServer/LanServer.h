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

		// ServerBase을(를) 통해 상속됨
		void OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type) override;
		void OnConnected(ULONGLONG sessionId) override;
		void OnDisconnected(ULONGLONG sessionId) override;

		void Init();
		void EndAction() override;

		void SetLobbySystem(class jh_content::LobbySystem* lobbySystem);
	private:

		//void InitPacketFuncs();
		//void HandleLanInfoNotifyPacket(ULONGLONG sessionId, PacketPtr& packet);
		//void HandleGameSettingRequestPacket(ULONGLONG sessionId, PacketPtr& packet);

		std::unique_ptr < jh_content::LobbyLanSystem> m_pLobbyLanSystem;
	};
}