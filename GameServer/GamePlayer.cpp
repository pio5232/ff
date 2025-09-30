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
alignas(64) std::atomic<int> jh_content::GamePlayer::aliveGamePlayerCount = 0;

jh_content::GamePlayer::GamePlayer(std::weak_ptr<class jh_content::User> ownerUser, GameWorld* worldPtr) : Player(worldPtr, EntityType::GamePlayer, posUpdateInterval), m_ownerUser(ownerUser),m_bWasInVictoryZone(false)
{
	aliveGamePlayerCount.fetch_add(1);
}

void jh_content::GamePlayer::Update(float delta)
{
	if (IsDead())
		return;

	Player::Update(delta);

	//printf("Player Position : [%0.3f, %0.3f, %0.3f]\n", m_transformComponent.GetPosConst().m_iX, m_transformComponent.GetPosConst().y, m_transformComponent.GetPosConst().m_iZ);

	//printf("Rotation Y : %0.3f     DirNormalized : [%0.3f, %0.3f, %0.3f]\n", m_transformComponent.GetRotConst().y, m_transformComponent.GetNormalizedDir().m_iX, m_transformComponent.GetNormalizedDir().y, m_transformComponent.GetNormalizedDir().m_iZ);

}

void jh_content::GamePlayer::MoveStart(const Vector3& clientMoveStartPos, float clientMoveStartRotY)
{
	//printf("Process Move Start [%0.3f, %0.3f, %0.3f] RotY : %f\n", clientPacket.pos.m_iX, clientPacket.pos.y, clientPacket.pos.m_iZ, clientPacket.rotY);
	CheckSync(clientMoveStartPos);

	m_transformComponent.SetDirection(clientMoveStartRotY);

	m_pStateController->ChangeState(&jh_content::PlayerMoveState::GetInstance());

	BroadcastMoveState();
}

void jh_content::GamePlayer::MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY)
{
	//printf("Process Move Stop [%0.3f, %0.3f, %0.3f] RotY : %f\n", clientPacket.pos.m_iX, clientPacket.pos.y, clientPacket.pos.m_iZ,clientPacket.rotY);
	//printf("Process Move Stop [%0.3f, %0.3f, %0.3f], Rot : %0.3f\n", clientPacket.pos.m_iX, clientPacket.pos.y, clientPacket.pos.m_iZ,clientPacket.rotY);

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

	//if (abs(serverPos.m_iX - clientPos.m_iX) < defaultErrorRange && abs(serverPos.m_iZ - clientPos.m_iZ) < defaultErrorRange)
	//if (Vector3::Distance(serverPos, clientPos) < defaultErrorRange)
		
	// 오차 범위가 작으면 무시한다. Move상태에서는 일정 주기마다 Update를 전송하고 있다.
	if ((serverPos - clientPos).sqrMagnitude() < (defaultErrorRange * defaultErrorRange))
		return;

	SendPositionUpdate();


	ULONGLONG entityId = GetEntityId();
	//const Vector3& syncRot = m_transformComponent.GetRotConst();

	//PacketPtr characterSyncPacket = jh_content::PacketBuilder::BuildCharacterSyncPacket(entityId, serverPos, syncRot);

	//m_pWorldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), characterSyncPacket);

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[CheckSync] - ID : [%llu] ,PlayerPos : [%0.3f, %0.3f, %0.3f], Position [%0.3f, %0.3f, %0.3f]", entityId, clientPos.x, clientPos.y, clientPos.z, serverPos.x, serverPos.y, serverPos.z);

	//printf("Sync!! ID - %llu ,PlayerPos [%0.3f, %0.3f, %0.3f], Position [%0.3f, %0.3f, %0.3f]\n", GetEntityId(), clientPos.x, clientPos.y, clientPos.z, serverPos.x, serverPos.y, serverPos.z);
}
