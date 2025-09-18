#pragma once
#include "PlayerStateBase.h"



namespace jh_content
{
	// -------------------------- Move --------------------------

	/// +-------------------+
	/// |  PlayerIdleState	|
	/// +-------------------+
	class PlayerIdleState : public PlayerMoveStateBase
	{
	public:
		static PlayerIdleState& GetInstance()
		{
			static PlayerIdleState instance;

			return instance;
		}

		PlayerIdleState(const PlayerIdleState&) = delete;
		PlayerIdleState& operator= (const PlayerIdleState&) = delete;

		virtual void OnStayState(jh_content::Player* player, float delta) override;

	private:
		PlayerIdleState();
	};

	/// +-------------------+
	/// |  PlayerMoveState	|
	/// +-------------------+
	class PlayerMoveState : public PlayerMoveStateBase
	{
	public:
		static PlayerMoveState& GetInstance()
		{
			static PlayerMoveState instance;

			return instance;
		}

		PlayerMoveState(const PlayerMoveState&) = delete;
		PlayerMoveState& operator= (const PlayerMoveState&) = delete;

		virtual void OnStayState(jh_content::Player* player, float delta) override;

	private:
		PlayerMoveState();
	};

	// -------------------------- Action --------------------------

	/// +-----------------------+
	/// |	PlayerAttackState	|
	/// +-----------------------+
	class PlayerAttackState : public PlayerActionStateBase
	{
	public:
		static PlayerAttackState& GetInstance()
		{
			static PlayerAttackState instance;

			return instance;
		}

		PlayerAttackState(const PlayerAttackState&) = delete;
		PlayerAttackState& operator= (const PlayerAttackState&) = delete;

		virtual void OnEnterState(jh_content::Player* player) override;
		virtual void OnStayState(jh_content::Player* player, float delta) override;
		virtual void OnExitState(jh_content::Player* player) override;

		virtual float GetDuration() const override { return attackDuration; }
	private:
		PlayerAttackState();
	};

	/// +-----------------------+
	/// |  PlayerAttackedState	|
	/// +-----------------------+
	class PlayerAttackedState : public PlayerActionStateBase
	{
	public:
		static PlayerAttackedState& GetInstance()
		{
			static PlayerAttackedState instance;

			return instance;
		}

		PlayerAttackedState(const PlayerAttackedState&) = delete;
		PlayerAttackedState& operator= (const PlayerAttackedState&) = delete;

		virtual void OnEnterState(jh_content::Player* player) override;
		virtual void OnStayState(jh_content::Player* player, float delta) override;
		virtual void OnExitState(jh_content::Player* player) override;

		virtual float GetDuration() const override { return attackedDuration; }

	private:
		PlayerAttackedState();
	};

	/// +-------------------+
	/// |  PlayerDeadState	|
	/// +-------------------+
	class PlayerDeadState : public PlayerActionStateBase
	{
	public:
		static PlayerDeadState& GetInstance()
		{
			static PlayerDeadState instance;

			return instance;
		}

		PlayerDeadState(const PlayerDeadState&) = delete;
		PlayerDeadState& operator= (const PlayerDeadState&) = delete;

		virtual void OnEnterState(jh_content::Player* player) override;
		virtual void OnStayState(jh_content::Player* player, float delta) override;
		virtual void OnExitState(jh_content::Player* player) override;

		virtual float GetDuration() const override { return DeadDuration; }

	private:
		PlayerDeadState();
	};

	/// +-------------------+
	/// |  NoneActionState	|
	/// +-------------------+}
	class NoneActionState : public PlayerActionStateBase
	{
	public:
		static NoneActionState& GetInstance()
		{
			static NoneActionState instance;

			return instance;
		}

		NoneActionState(const NoneActionState&) = delete;
		NoneActionState& operator= (const NoneActionState&) = delete;

		virtual void OnEnterState(jh_content::Player* player) override;
		virtual void OnStayState(jh_content::Player* player, float delta) override;
		virtual void OnExitState(jh_content::Player* player) override;
	private:
		NoneActionState();
	};
}