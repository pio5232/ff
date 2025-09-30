#pragma once
#include "Player.h"

namespace jh_content
{
	// Session - Network / User - Contents 영역.
	class GameWorld;
	class GamePlayer : public Player
	{
	public:
		GamePlayer(std::weak_ptr<class jh_content::User> ownerUser, GameWorld* worldPtr); // User.
		~GamePlayer() { aliveGamePlayerCount.fetch_sub(1); }

		virtual void Update(float delta);

		static ULONGLONG GetAliveGamePlayerCount() { return aliveGamePlayerCount.load(); }
		//ULONGLONG GetUserId() const { return _userId; }
		//GameSessionPtr GetOwnerSession() { return _ownerSession.lock(); }

		// process packet data
		// 이런 함수 만들고 전송하는 부분 모두 분리하도록 한다.
		void MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY);
		void MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY);

		void SetAttackState();

		bool GetWasInVictoryZone() const { return m_bWasInVictoryZone; }
		void SetWasInVictoryZone(bool value) { m_bWasInVictoryZone = value; }

		UserPtr GetOwnerUser() const { return m_ownerUser.lock(); }
		
	private:
		static alignas(64) std::atomic<int> aliveGamePlayerCount;

		void CheckSync(const Vector3& clientPos);
		std::weak_ptr<class jh_content::User> m_ownerUser;

		bool m_bWasInVictoryZone;
	};
}