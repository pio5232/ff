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

		//ULONGLONG GetUserId() const { return _userId; }
		//GameSessionPtr GetOwnerSession() { return _ownerSession.lock(); }

		// process packet data
		// 이런 함수 만들고 전송하는 부분 모두 분리하도록 한다.
		void MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY);
		void MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY);

		void SetAttackState();
		static int GetAliveGamePlayerCount() { return aliveGamePlayerCount.load(); }

		bool GetWasInVictoryZone() const { return m_bWasInVictoryZone; }
		void SetWasInVictoryZone(bool value) { m_bWasInVictoryZone = value; }

		UserPtr GetOwnerUser() const { return m_ownerUser.lock(); }
		
		static alignas(64) std::atomic<int> aliveGamePlayerCount;
	private:
		//static std::atomic<int> aliveGamePlayerCount;

		//void SyncPos(const Vector3& clientPos);
		std::weak_ptr<class jh_content::User> m_ownerUser;

		bool m_bWasInVictoryZone;
	};
}