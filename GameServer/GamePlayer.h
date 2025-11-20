#pragma once
#include "Player.h"

namespace jh_content
{
	class GameWorld;
	class GamePlayer : public Player
	{
	public:
		GamePlayer(std::weak_ptr<class jh_content::User> ownerUser, GameWorld* worldPtr); 
		~GamePlayer() { InterlockedDecrement(&aliveGamePlayerCount); }

		virtual void Update(float delta);

		void MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY);
		void MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY);

		void SetAttackState();

		void SetWasInVictoryZone(bool value) { m_bWasInVictoryZone = value; }
		
		bool GetWasInVictoryZone() const			{ return m_bWasInVictoryZone; }
		UserPtr GetOwnerUser() const				{ return m_ownerUser.lock(); }
		
		static ULONGLONG GetAliveGamePlayerCount()	{ return aliveGamePlayerCount; }
	private:
		static alignas(32) volatile LONG		aliveGamePlayerCount;
		
		void CheckSync(const Vector3& clientPos);
		std::weak_ptr<class jh_content::User>	m_ownerUser;

		bool									m_bWasInVictoryZone;
	};
}