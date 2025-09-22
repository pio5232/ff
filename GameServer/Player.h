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
		bool IsDead() const override { return m_statComponent.IsDead(); }
		float GetAttackRange() const { return m_statComponent.GetAttackRange(); }
		virtual bool IsMoving() const override;
		virtual void TakeDamage(USHORT damage) override;
		virtual USHORT GetHp() const override { return m_statComponent.GetHp(); }
		USHORT GetAttackDamage() const { return m_statComponent.GetAttackDamage(); }
	protected:
		void BroadcastMoveState();
		void SendPositionUpdate();

	protected:
		StatComponent m_statComponent;
		std::unique_ptr<PlayerStateController> m_pStateController;

		float m_fPosUpdateInterval;
	private:
		Vector3 m_lastUpdatePos;
		class GameWorld* m_pWorldPtr;

	};
}