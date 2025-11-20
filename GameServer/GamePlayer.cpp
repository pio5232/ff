#include "pch.h"
#include <sstream>
#include <math.h>
#include "GamePlayer.h"
#include "Player.h"
#include "UserManager.h"
#include "PlayerStateController.h"
#include "PlayerState.h"
#include "PacketBuilder.h"
#include "GameWorld.h"

using namespace jh_network;
alignas(32) volatile LONG jh_content::GamePlayer::aliveGamePlayerCount = 0;

jh_content::GamePlayer::GamePlayer(std::weak_ptr<class jh_content::User> ownerUser, GameWorld* worldPtr) : Player(worldPtr, EntityType::GamePlayer, posUpdateInterval), m_ownerUser(ownerUser),m_bWasInVictoryZone(false)
{
	InterlockedIncrement(&aliveGamePlayerCount);
}

void jh_content::GamePlayer::Update(float delta)
{
	if (IsDead())
		return;

	Player::Update(delta);
}

void jh_content::GamePlayer::MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY)
{
	CheckSync(clientMoveStartPos);

	m_transformComponent.SetDirection(clientMoveStartRotY);

	m_pStateController->ChangeState(&jh_content::PlayerMoveState::GetInstance());

	BroadcastMoveState();
}

void jh_content::GamePlayer::MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY)
{
	CheckSync(clientMoveStopPos);

	m_transformComponent.SetDirection(clientMoveStopRotY);

	m_pStateController->ChangeState(&jh_content::PlayerIdleState::GetInstance());

	BroadcastMoveState();
}


void jh_content::GamePlayer::SetAttackState()
{
	m_pStateController->ChangeState(&jh_content::PlayerAttackState::GetInstance());
}


void jh_content::GamePlayer::CheckSync(const Vector3& clientPos)
{
	const Vector3& serverPos = m_transformComponent.GetPosConst();

	// 오차 범위가 작으면 무시한다. Move상태에서는 일정 주기마다 Update를 전송하고 있다.
	if ((serverPos - clientPos).sqrMagnitude() < (defaultErrorRange * defaultErrorRange))
		return;

	SendPositionUpdate();

	ULONGLONG entityId = GetEntityId();
	
	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_DEBUG, L"[CheckSync] EntityId: [%llu], ClientPos: [%0.3f, %0.3f, %0.3f], ServerPos: [%0.3f, %0.3f, %0.3f]", entityId, clientPos.x, clientPos.y, clientPos.z, serverPos.x, serverPos.y, serverPos.z);

}
