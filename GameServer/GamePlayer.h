#pragma once
#include "Player.h"

namespace jh_content
{
	// Session - Network / User - Contents ����.
	class GameWorld;
	class GamePlayer : public Player
	{
	public:
		GamePlayer(UserPtr ownerUser, GameWorld* worldPtr); // User.
		~GamePlayer() { m_aliveGamePlayerCount.fetch_sub(1); }

		virtual void Update(float delta);

		//ULONGLONG GetUserId() const { return _userId; }
		//GameSessionPtr GetOwnerSession() { return _ownerSession.lock(); }

		// process packet data
		// �̷� �Լ� ����� �����ϴ� �κ� ��� �и��ϵ��� �Ѵ�.
		void MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY);
		void MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY);

		void SetAttackState();
		static int GetAliveGamePlayerCount() { return m_aliveGamePlayerCount.load(); }

		bool GetWasInVictoryZone() const { return m_bWasInVictoryZone; }
		void SetWasInVictoryZone(bool value) { m_bWasInVictoryZone = value; }
	private:
		static std::atomic<int> m_aliveGamePlayerCount;

		void SyncPos(const Vector3& clientPos);
		std::weak_ptr<class jh_content::User> m_ownerUser;

		bool m_bWasInVictoryZone;
	};
}