#pragma once

namespace jh_content
{
	class Player;

	class PlayerMoveStateBase
	{
	public:
		enum class MoveState : byte
		{
			None = 0,
			Idle,
			Move,
		};
		PlayerMoveStateBase(MoveState state) : m_moveState(state) {}

		virtual void OnStayState(jh_content::Player* player, float delta) = 0;

		MoveState GetType() const { return m_moveState; }
	private:
		MoveState m_moveState;
	};

	class PlayerActionStateBase
	{
	public:
		enum class ActionState
		{
			None = 0,
			Attack,
			Attacked,
			Dead,
		};
		PlayerActionStateBase(ActionState state) : m_actionState(state) {}
		virtual void OnEnterState(jh_content::Player* player) = 0;
		virtual void OnStayState(jh_content::Player* player, float delta) = 0;
		virtual void OnExitState(jh_content::Player* player) = 0;

		virtual float GetDuration() const { return 0; }
		ActionState GetType() const { return m_actionState; }
	private:
		ActionState m_actionState;
	};
}