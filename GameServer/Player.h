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
		bool IsDead() const override { return _statComponent.IsDead(); }
		float GetAttackRange() const { return _statComponent.GetAttackRange(); }
		virtual bool IsMoving() const override;
		virtual void TakeDamage(USHORT damage) override;
		virtual USHORT GetHp() const override { return _statComponent.GetHp(); }
		USHORT GetAttackDamage() const { return _statComponent.GetAttackDamage(); }
	protected:
		void BroadcastMoveState();
		void SendPositionUpdate();

	protected:
		StatComponent _statComponent;
		std::unique_ptr<PlayerStateController> _stateController;

		float _posUpdateInterval;
	private:
		Vector3 _lastUpdatePos;
		class GameWorld* _worldPtr;

	};
}