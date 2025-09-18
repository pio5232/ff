#pragma once

namespace jh_network
{
	class LobbyLanServer;
}
namespace jh_content
{
	class LobbySystem;
	class LobbyServer;
	class RoomManager;
	class UserManager;
}

#define LOBBY_TIMEOUT_CHECK_INTERVAL 5000 // 체크 주기 5초

#define LOBBY_CATEGORY_NAME L"LobbyServer"
#define LOBBY_DATA_CATEGORY_NAME L"LobbyData"
#define LAN_CATEGORY_NAME L"LanServer"

#define USER_MANAGER_SAVE_FILE_NAME L"UserManager"

#define	LOBBY_DATA_CONFIG_FILE L"LobbyData.cfg"
#define LOBBY_SERVER_CONFIG_FILE L"LobbyServer.cfg"

#define LAN_DATA_CONFIG_FILE L"LanData.cfg"
#define LAN_SERVER_CONFIG_FILE L"LanServer.cfg"

#define LOBBY_SERVER_SAVE_FILE_NAME L"LobbyServer"
#define LOBBY_SYSTEM_SAVE_FILE_NAME L"LobbySystem"

#define LAN_SAVE_FILE_NAME L"LobbyLanServer"

#define ROOM_MANAGER_SAVE_FILE_NAME L"RoomManager"
#define USER_MANAGER_SAVE_FILE_NAME L"UserManager"

enum class LanRequestMsgType : byte
{
	NONE = 0,
	GAME_SETTING_REQUEST,
	LAN_INFO_NOTIFY
};

struct LanRequest
{
	explicit LanRequest(ULONGLONG lanSessionId, LanRequestMsgType msgType, PacketPtr packet, jh_network::IocpServer* lanServer) : m_ullSessionId(lanSessionId), m_lanRequestMsgType(msgType), m_pPacket(packet), m_pLanServer(lanServer) {}
	~LanRequest()
	{
		m_ullSessionId = INVALID_SESSION_ID;
		m_lanRequestMsgType = LanRequestMsgType::NONE;
		m_pPacket.reset();
		m_pLanServer = nullptr;
	}

	LanRequest(const LanRequest& other) = default;
	LanRequest(LanRequest&& other) = default;

	LanRequest& operator=(const LanRequest& other) = default;
	LanRequest& operator=(LanRequest&& other) = default;

	ULONGLONG m_ullSessionId;
	LanRequestMsgType m_lanRequestMsgType;
	PacketPtr m_pPacket;
	jh_network::IocpServer* m_pLanServer;
};

using LanRequestPtr = std::shared_ptr<LanRequest>;

using UserPtr = std::shared_ptr<class jh_content::User>;
using RoomPtr = std::shared_ptr<class jh_content::Room>;
