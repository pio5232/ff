#pragma once

namespace jh_network
{
	class GameSession;
	class LanClientSession;
}


#define GAME_SERVER_CONFIG_FILE L"GameServer.cfg"
#define GAME_LAN_CLIENT_CONFIG_FILE L"GameLanClient.cfg"

#define GAME_CATEGORY_NAME L"GameServer"
#define GAME_LAN_CATEGORY_NAME L"GameLanClient"

#define GAME_SERVER_SAVE_FILE_NAME L"GameServer"
#define GAME_SYSTEM_SAVE_FILE_NAME L"GameSystem"
#define GAME_LAN_CLIENT_SAVE_FILE_NAME L"GameLanClient"

#define GAME_USER_MANAGER_SAVE_FILE_NAME L"UserManager"

using SendPacketFunc		= std::function<void(ULONGLONG, PacketBufferRef&)>; // [sessionId, packet]
using GameSessionPtr		= std::shared_ptr<jh_network::GameSession>;
using LanClientSessionPtr	= std::shared_ptr<jh_network::LanClientSession>;

constexpr int fixedFrame			= 60;
constexpr float fixedDeltaTime		= 1.0f / fixedFrame;

constexpr float limitDeltaTime		= 0.2f;

constexpr float VictoryZoneMinX		= 45.0f;
constexpr float VictoryZoneMaxX		= 55.0f;

constexpr float VictoryZoneMinZ		= 45.0f;
constexpr float VictoryZoneMaxZ		= 55.0f;

constexpr float COS_30				= 0.8660254f;

constexpr float mapXMin				= 0;
constexpr float mapXMax				= 100.0f;
constexpr float mapZMin				= 0;
constexpr float mapZMax				= 100.0f;

constexpr float centerX				= (mapXMax - mapXMin) / 2.0f;
constexpr float centerZ				= (mapZMax - mapZMin) / 2.0f;
constexpr int sectorCriteriaSize	= 10;

constexpr int sectorMaxX			= ((int)mapXMax - (int)mapXMin) / sectorCriteriaSize + 2; // 0 [ 1 2 3 4 5 ] 6
constexpr int sectorMaxZ			= ((int)mapZMax - (int)mapZMin) / sectorCriteriaSize + 2;

// for문 사용
// for( i = startX/Z; i<endX/Z; i++)
constexpr int startXSectorPos					= 1;
constexpr int startZSectorPos					= 1;
constexpr int endXSectorPos						= sectorMaxX - 1;
constexpr int endZSectorPos						= sectorMaxZ - 1;

constexpr float edgeThreshold					= 10.0f;

constexpr float defaultSlowWalkSpeed			= 3.0f;
constexpr float defaultWalkSpeed				= 6.0f;
constexpr float defaultRunSpeed					= 9.0f;

constexpr USHORT defaultMaxHp					= 3;
constexpr USHORT defaultAttackDamage			= 1;

constexpr float defaultAttackRange				= 1.6f;

constexpr float defaultErrorRange				= 0.5f; // 오차 범위
constexpr float posUpdateInterval				= 0.2f;

constexpr float attackDuration					= 0.8f;
constexpr float attackedDuration				= 1.0f;
constexpr float DeadDuration					= 3.0f;

constexpr ULONGLONG victoryZoneCheckDuration	= 10000; // 이름 바꾸기. 존 진입 시 우승자가 되기 위해 견뎌야하는 시간 (ms)

namespace jh_content
{
	class Entity;
	class GamePlayer;
	class AIPlayer;
	class WorldChat;
}

struct TimerAction
{
	ULONGLONG executeTick;
	Action action;

	const bool operator< (const TimerAction& other) const
	{
		return executeTick > other.executeTick;
	}
};


Vector3 GenerateRandomPos();

enum class GameLanRequestMsgType
{
	NONE = 0,
};

struct GameLanRequest
{
	explicit GameLanRequest(ULONGLONG lanSessionId, USHORT msgType, PacketBufferRef packet, jh_network::IocpClient* lanClient) : m_ullSessionId(lanSessionId), m_usMsgType(msgType), m_pPacket(packet), m_pClient(lanClient) {}
	~GameLanRequest()
	{
		m_ullSessionId = INVALID_SESSION_ID;
		m_usMsgType = jh_network::INVALID_PACKET;
		m_pPacket.reset();
		m_pClient = nullptr;
	}

	GameLanRequest(const GameLanRequest& other) = default;
	GameLanRequest(GameLanRequest&& other) = default;

	GameLanRequest& operator=(const GameLanRequest& other) = default;
	GameLanRequest& operator=(GameLanRequest&& other) = default;

	ULONGLONG				m_ullSessionId;
	USHORT					m_usMsgType;
	PacketBufferRef			m_pPacket;
	jh_network::IocpClient	* m_pClient;
};

using UserPtr = std::shared_ptr<class jh_content::User>;

using GamePlayerPtr = std::shared_ptr<class jh_content::GamePlayer>;
using AIPlayerPtr = std::shared_ptr<class jh_content::AIPlayer>;
using EntityPtr = std::shared_ptr<class jh_content::Entity>;

using WorldChatPtr = std::shared_ptr<class jh_content::WorldChat>;
using Action = std::function<void()>;

using GameLanRequestPtr = std::shared_ptr<GameLanRequest>;