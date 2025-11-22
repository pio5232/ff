#pragma once

#include "NetworkBase.h"
#include "NetworkUtils.h"

namespace jh_content
{
    class LobbyLanServer;
    class LobbySystem;

    class LobbyServer : public jh_network::IocpServer
    {
    public:
        LobbyServer();
        ~LobbyServer();

        void OnRecv(ULONGLONG sessionId, PacketBufferRef packet, USHORT type) override;
        void OnConnected(ULONGLONG sessionId) override;
        void OnDisconnected(ULONGLONG sessionId) override;

        void Monitor();
        void GetInvalidMsgCnt();
    private:
        void BeginAction() override;
        void EndAction() override;

        std::unique_ptr<class jh_content::LobbyLanServer>   m_pLanServer;
        std::unique_ptr<class jh_content::LobbySystem>      m_pLobbySystem;
    };
}