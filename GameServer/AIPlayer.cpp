#include "pch.h"
#include <random>
#include "AIPlayer.h"
#include "UserManager.h"
#include "PlayerStateController.h"
#include "PlayerState.h"
#include "GameWorld.h"
jh_content::AIPlayer::AIPlayer(GameWorld* worldPtr) : Player(worldPtr, EntityType::AIPlayer, posUpdateInterval), m_fMovementUpdateInterval(0)
{
}

void jh_content::AIPlayer::Update(float delta)
{
	if (IsDead())
		return;

	m_fMovementUpdateInterval -= delta;

	if (m_fMovementUpdateInterval <= 0)
	{
		m_fMovementUpdateInterval = static_cast<float>(GetRandDouble(1.0, 6.0, 3));

		UpdateAIMovement();
	}

	Player::Update(delta);
}

void jh_content::AIPlayer::UpdateAIMovement()
{
	if (CheckChance(70))
	{
		m_transformComponent.SetRandomDirection();
		m_pStateController->ChangeState(&jh_content::PlayerMoveState::GetInstance());
	}
	else
	{
		m_pStateController->ChangeState(&jh_content::PlayerIdleState::GetInstance());
	}

	BroadcastMoveState();
}

