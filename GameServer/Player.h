#pragma once
#include "StatComponent.h"
#include "Entity.h"

namespace jh_content
{
	class GameWorld;
	class PlayerStateController;

	class Player : public Entity
	{
	public:
		Player(class GameWorld* worldPtr, EntityType type, float updateInterval);
		~Player();

		virtual void Update(float delta) = 0;

		void Move(float delta);

		virtual bool IsMoving() const override;
		virtual void TakeDamage(USHORT damage) override;
		virtual USHORT GetHp() const override				{ return m_statComponent.GetHp(); }
		virtual bool IsDead() const override				{ return m_statComponent.IsDead(); }
		
		USHORT GetAttackDamage() const						{ return m_statComponent.GetAttackDamage(); }
		float GetAttackRange() const						{ return m_statComponent.GetAttackRange(); }
	protected:
		void BroadcastMoveState();
		void SendPositionUpdate();

	protected:
		StatComponent							m_statComponent;
		std::unique_ptr<PlayerStateController>	m_pStateController;

		float									m_fPosUpdateInterval;
		class GameWorld							* m_pWorldPtr;
	private:
		Vector3									m_lastUpdatePos;

	};
}