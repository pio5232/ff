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

	private:
		std::unique_ptr<class jh_content::GameSystem> m_pGameSystem;
		std::unique_ptr<class jh_content::GameLanClient> m_pGameLanClient;




	public:
		/*void Init(USHORT roomNumber, USHORT requiredUsers, USHORT m_usMaxUserCnt);
		void LanClientConnect(const NetAddress& netAddr);
		void LanClientDisconnect();

		USHORT GetRoomNumber() const { return _gameInfo.m_usRoomNumber; }
		ULONGLONG GetToken() const { return _gameInfo.m_ullEnterToken; }
		USHORT GetRealPort() const { return _gameInfo.m_usPort; }
		USHORT GetRequiredUsers() const { return _gameInfo.m_usRequiredUserCnt; }
		USHORT MaxUsers() const { return _gameInfo.m_usMaxUserCnt; }

		ErrorCode TryRun();

		void CheckLoadingAndStartLogic();

		GamePlayerPtr CreateUser(GameSessionPtr gameSessionPtr);
		ErrorCode RemoveUser(GamePlayerPtr gamePlayerPtr);

		void EnqueueAction(Action&& action, bool mustEnqueue = false);
		void EnqueueTimerAction(TimerAction&& timerAction);
		WorldChatPtr GetWorldChatPtr();

		void ProxyAttackPacket(GameSessionPtr gameSession, AttackRequestPacket packet);

	private:
		std::shared_ptr<class LanClient> m_pLanServer;

		std::unique_ptr<class jh_content::GameWorld> _gameWorld;
		GameServerInfo _gameInfo;

		std::atomic<bool> _isRunning;
		std::atomic<int> _loadCompletedCnt;

		void CheckHeartbeat();
		volatile bool _canCheckHeartbeat;
		std::thread _heartbeatCheckThread;*/

	};

}