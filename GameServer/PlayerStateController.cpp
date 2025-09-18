#include "pch.h"
#include "Player.h"
#include "PlayerStateController.h"
#include "PlayerState.h"
/// +-----------------------+
///	| PlayerStateController |
/// +-----------------------+
/// 
jh_content::PlayerStateController::PlayerStateController(jh_content::Player* player) : _player(player), _actionStateBasePtr(&NoneActionState::GetInstance()),
_moveStateBasePtr(nullptr), _actionElapsedTime(0)
{
}

void jh_content::PlayerStateController::ChangeState(PlayerMoveStateBase* to)
{
	if (to == nullptr || to == _moveStateBasePtr)
		return;

	_moveStateBasePtr = to;
}

void jh_content::PlayerStateController::ChangeState(PlayerActionStateBase* to)
{
	if (to == nullptr || to == _actionStateBasePtr)
		return;

	_actionElapsedTime = 0;

	_actionStateBasePtr->OnExitState(_player);
	_actionStateBasePtr = to;
	_actionStateBasePtr->OnEnterState(_player);
}

void jh_content::PlayerStateController::Update(float delta)
{
	if (_moveStateBasePtr != nullptr)
		_moveStateBasePtr->OnStayState(_player, delta);

	if (_actionStateBasePtr != nullptr)
	{
		if (_actionStateBasePtr->GetType() == jh_content::PlayerActionStateBase::ActionState::None)
			return;

		_actionStateBasePtr->OnStayState(_player, delta);


		_actionElapsedTime += delta;

		if (_actionElapsedTime >= _actionStateBasePtr->GetDuration())
			ChangeState(&jh_content::NoneActionState::GetInstance());
	}

}

jh_content::PlayerMoveStateBase::MoveState jh_content::PlayerStateController::GetMoveType()
{
	if (_moveStateBasePtr == nullptr)
		return jh_content::PlayerMoveStateBase::MoveState::None;

	return _moveStateBasePtr->GetType();
}

jh_content::PlayerActionStateBase::ActionState jh_content::PlayerStateController::GetActionType()
{
	if (_actionStateBasePtr == nullptr)
		return PlayerActionStateBase::ActionState::None;

	return _actionStateBasePtr->GetType();
}
