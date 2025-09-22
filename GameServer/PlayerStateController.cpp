#include "pch.h"
#include "Player.h"
#include "PlayerStateController.h"
#include "PlayerState.h"
/// +-----------------------+
///	| PlayerStateController |
/// +-----------------------+
/// 
jh_content::PlayerStateController::PlayerStateController(jh_content::Player* player) : m_pPlayer(player), m_pActionStateBasePtr(&NoneActionState::GetInstance()),
m_pMoveStateBasePtr(nullptr), m_fActionElapsedTime(0)
{
}

void jh_content::PlayerStateController::ChangeState(PlayerMoveStateBase* to)
{
	if (to == nullptr || to == m_pMoveStateBasePtr)
		return;

	m_pMoveStateBasePtr = to;
}

void jh_content::PlayerStateController::ChangeState(PlayerActionStateBase* to)
{
	if (to == nullptr || to == m_pActionStateBasePtr)
		return;

	m_fActionElapsedTime = 0;

	m_pActionStateBasePtr->OnExitState(m_pPlayer);
	m_pActionStateBasePtr = to;
	m_pActionStateBasePtr->OnEnterState(m_pPlayer);
}

void jh_content::PlayerStateController::Update(float delta)
{
	if (m_pMoveStateBasePtr != nullptr)
		m_pMoveStateBasePtr->OnStayState(m_pPlayer, delta);

	if (m_pActionStateBasePtr != nullptr)
	{
		if (m_pActionStateBasePtr->GetType() == jh_content::PlayerActionStateBase::ActionState::None)
			return;

		m_pActionStateBasePtr->OnStayState(m_pPlayer, delta);


		m_fActionElapsedTime += delta;

		if (m_fActionElapsedTime >= m_pActionStateBasePtr->GetDuration())
			ChangeState(&jh_content::NoneActionState::GetInstance());
	}

}

jh_content::PlayerMoveStateBase::MoveState jh_content::PlayerStateController::GetMoveType()
{
	if (m_pMoveStateBasePtr == nullptr)
		return jh_content::PlayerMoveStateBase::MoveState::None;

	return m_pMoveStateBasePtr->GetType();
}

jh_content::PlayerActionStateBase::ActionState jh_content::PlayerStateController::GetActionType()
{
	if (m_pActionStateBasePtr == nullptr)
		return PlayerActionStateBase::ActionState::None;

	return m_pActionStateBasePtr->GetType();
}
