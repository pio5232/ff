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
		jh_content::Player* _player;

		jh_content::PlayerActionStateBase* _actionStateBasePtr;
		jh_content::PlayerMoveStateBase* _moveStateBasePtr;

		float _actionElapsedTime;
	};
}

