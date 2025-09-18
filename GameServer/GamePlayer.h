#pragma once
#include "Player.h"

namespace jh_content
{
	// Session - Network / User - Contents ����.
	class GameWorld;
	class GamePlayer : public Player
	{
	public:
		GamePlayer(GameSessionPtr gameSessionPtr, GameWorld* worldPtr); // User.
		~GamePlayer() { _aliveGamePlayerCount.fetch_sub(1); }

		virtual void Update(float delta);

		//ULONGLONG GetUserId() const { return _userId; }
		//GameSessionPtr GetOwnerSession() { return _ownerSession.lock(); }

		// process packet data
		// �̷� �Լ� ����� �����ϴ� �κ� ��� �и��ϵ��� �Ѵ�.
		void MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY);
		void MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY);

		void SetAttackState();
		static int GetAliveGamePlayerCount() { return _aliveGamePlayerCount.load(); }

		bool GetWasInVictoryZone() const { return _wasInVictoryZone; }
		void SetWasInVictoryZone(bool value) { _wasInVictoryZone = value; }
	private:
		static std::atomic<int> _aliveGamePlayerCount;

		void SyncPos(const Vector3& clientPos);

		std::weak_ptr<class jh_network::GameSession> _ownerSession;

		bool _wasInVictoryZone;
	};
}