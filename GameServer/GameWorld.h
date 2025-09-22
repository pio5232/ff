#pragma once
#include "Sector.h"

namespace jh_content
{
	class GameWorld
	{
	public:

		using SendPacketFunc = std::function<void(ULONGLONG, PacketPtr&)>; // sessionId

		GameWorld(UserManager* userManager, SendPacketFunc sendPacketFunc);
		~GameWorld();

		void StartGame();
		void Stop();

		void Update(float deltaTime);

		bool TryEnqueueTimerAction(TimerAction&& action);
		void ProcessTimerActions();

		void AddEntity(EntityPtr entityPtr);
		void RemoveEntity(ULONGLONG entityId);

		// 게임 시작 관련 초기화
		void Init(USHORT total, USHORT gamePlayerCount); // 1. ai+player, 2. player

		//WorldChatPtr GetWorldChat() { return _worldChat; }

		void SendPacketAroundSectorNSpectators(const Sector& sector, PacketPtr packet);
		void SendPacketAroundSectorNSpectators(int sectorX, int sectorZ, PacketPtr packet);

		void CleanUpSpectatorEntities();
		void SendToSpectatorEntities(PacketPtr packet);

		void SetSpectator(EntityPtr entity);
		const class SectorManager* GetSectorManagerConst() const { return m_pSectorManager.get(); }

		void HandleAttackPacket(GamePlayerPtr attacker, const jh_network::AttackRequestPacket& packet);

		void CreateAI(class jh_content::GameWorld* worldPtr);
		GamePlayerPtr CreateGamePlayer();

	public:
		void CheckVictoryZoneEntry(GamePlayerPtr gamePlayerPtr);
		//void OnEnterVictoryZone(GamePlayerPtr gamePlayerPtr);
		void CheckWinner();
		void InvalidateWinner(ULONGLONG userId);
	private:
		void SetDSCount(USHORT predMaxCnt);
		bool IsInVictoryZone(const Vector3& pos) const;
		
		ULONGLONG m_ullExpectedWinnerId = 0;
		ULONGLONG m_ullExpectedWinTime = MAXULONGLONG;
		volatile bool m_bIsGameRunning;

	private:
		SendPacketFunc m_sendPacketFunc;
		std::atomic<bool> m_bIsUpdateRunning;

		float m_fDeltaSum;

		std::priority_queue<TimerAction> m_timerActionQueue;

		// [Entity_ID, shared_ptr<Entity>]
		std::unordered_map<ULONGLONG, EntityPtr> m_aliveEntityDic;
		// [shared_ptr<Entity>, vectorIndex] - 
		std::unordered_map<EntityPtr, int> m_aliveEntityToVectorIdxDic;
		std::vector<EntityPtr> m_aliveEntityArr;

		std::vector<std::weak_ptr<jh_content::Entity>> m_spectatorEntityArr;

		std::unique_ptr<class SectorManager> m_pSectorManager;
		class UserManager* m_pUserManager;
	};
}
