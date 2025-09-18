#include "pch.h"
#include "PlayerStateBase.h"
#include "PlayerState.h"
#include "Player.h"
/// +-------------------+
/// |  PlayerIdleState	|
/// +-------------------+
/// 

jh_content::PlayerIdleState::PlayerIdleState() : PlayerMoveStateBase(MoveState::Idle) {}

void jh_content::PlayerIdleState::OnStayState(jh_content::Player* player, float delta)
{
}


/// +-------------------+
/// |  PlayerMoveState	|
/// +-------------------+
/// 
jh_content::PlayerMoveState::PlayerMoveState() : PlayerMoveStateBase(MoveState::Move) {}

void jh_content::PlayerMoveState::OnStayState(jh_content::Player* player, float delta)
{
	player->Move(delta);

}

/// +-----------------------+
/// |	PlayerAttackState	|
/// +-----------------------+
/// 
jh_content::PlayerAttackState::PlayerAttackState() : PlayerActionStateBase(ActionState::Attack) {}

void jh_content::PlayerAttackState::OnEnterState(jh_content::Player* player)
{
}

void jh_content::PlayerAttackState::OnStayState(jh_content::Player* player, float delta)
{
}

void jh_content::PlayerAttackState::OnExitState(jh_content::Player* player)
{
}

/// +-----------------------+
/// |  PlayerAttackedState	|
/// +-----------------------+
/// 
jh_content::PlayerAttackedState::PlayerAttackedState() : PlayerActionStateBase(ActionState::Attacked) {}

void jh_content::PlayerAttackedState::OnEnterState(jh_content::Player* player)
{
}

void jh_content::PlayerAttackedState::OnStayState(jh_content::Player* player, float delta)
{
}

void jh_content::PlayerAttackedState::OnExitState(jh_content::Player* player)
{
}

/// +-------------------+
/// |  PlayerDeadState	|
/// +-------------------+
jh_content::PlayerDeadState::PlayerDeadState() : PlayerActionStateBase(ActionState::Dead) {}

void jh_content::PlayerDeadState::OnEnterState(jh_content::Player* player)
{
}

void jh_content::PlayerDeadState::OnStayState(jh_content::Player* player, float delta)
{
}

void jh_content::PlayerDeadState::OnExitState(jh_content::Player* player)
{
}

/// +-------------------+
/// |  NoneActionState	|
/// +-------------------+}

void jh_content::NoneActionState::OnEnterState(jh_content::Player* player) {}

void jh_content::NoneActionState::OnStayState(jh_content::Player* player, float delta) {}

void jh_content::NoneActionState::OnExitState(jh_content::Player* player) {}

jh_content::NoneActionState::NoneActionState() : PlayerActionStateBase(ActionState::None) {}

