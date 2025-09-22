#pragma once

#include "PlayerStateBase.h"

namespace jh_content
{
	class Player;

	/// +-----------------------+
	///	| PlayerStateController |
	/// +-----------------------+
	class PlayerStateController
	{
	public:
		PlayerStateController(Player* player);

		void ChangeState(PlayerMoveStateBase* to);
		void ChangeState(PlayerActionStateBase* to);

		void Update(float delta);
		PlayerMoveStateBase::MoveState GetMoveType();
		PlayerActionStateBase::ActionState GetActionType();
	private:
		// �����Ҵ������ʴ� ������.
		jh_content::Player* m_pPlayer;

		jh_content::PlayerActionStateBase* m_pActionStateBasePtr;
		jh_content::PlayerMoveStateBase* m_pMoveStateBasePtr;

		float m_fActionElapsedTime;
	};
}

