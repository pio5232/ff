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

std::atomic<int> jh_content::GamePlayer::m_aliveGamePlayerCount;

using namespace jh_network;
jh_content::GamePlayer::GamePlayer(UserPtr ownerUser, GameWorld* worldPtr) : Player(worldPtr, EntityType::GamePlayer, posUpdateInterval), m_ownerUser(ownerUser),m_bWasInVictoryZone(false)
{
	m_aliveGamePlayerCount.fetch_add(1);
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
	SyncPos(clientMoveStartPos);

	m_transformComponent.SetDirection(clientMoveStartRotY);

	m_pStateController->ChangeState(&jh_content::PlayerMoveState::GetInstance());

	BroadcastMoveState();
}

void jh_content::GamePlayer::MoveStop(const Vector3& clientMoveStopPos, float clientMoveStopRotY)
{
	//printf("Process Move Stop [%0.3f, %0.3f, %0.3f] RotY : %f\n", clientPacket.pos.m_iX, clientPacket.pos.y, clientPacket.pos.m_iZ,clientPacket.rotY);
	//printf("Process Move Stop [%0.3f, %0.3f, %0.3f], Rot : %0.3f\n", clientPacket.pos.m_iX, clientPacket.pos.y, clientPacket.pos.m_iZ,clientPacket.rotY);

	//SyncPos(clientMoveStopPos);

	m_transformComponent.SetDirection(clientMoveStopRotY);

	m_pStateController->ChangeState(&jh_content::PlayerIdleState::GetInstance());

	BroadcastMoveState();
}


void jh_content::GamePlayer::SetAttackState()
{
	m_pStateController->ChangeState(&jh_content::PlayerAttackState::GetInstance());
}


void jh_content::GamePlayer::SyncPos(const Vector3& clientPos)
{
//	const Vector3& serverPos = m_transformComponent.GetPosConst();
//
//	//if (abs(serverPos.m_iX - clientPos.m_iX) < defaultErrorRange && abs(serverPos.m_iZ - clientPos.m_iZ) < defaultErrorRange)
//	//if (Vector3::Distance(serverPos, clientPos) < defaultErrorRange)
//	if ((serverPos - clientPos).sqrMagnitude() < (defaultErrorRange * defaultErrorRange))
//	{
//		// 오차 범위가 작으면 start / stop 나올 때 clientPos를 기준으로 설정. 
//		// 
//		m_transformComponent.SetPosition(clientPos);
//		return;
//	}
//
//	ULONGLONG entityId = GetEntityId();
//	const Vector3& syncRot = m_transformComponent.GetRotConst();
//
//	PacketPtr buffer = jh_content::PacketBuilder::BuildCharacterSyncPacket(entityId, serverPos, syncRot);
//
//	m_pWorld->Broadcast(buffer);
//	printf("Sync!! ID - %llu ,PlayerPos [%0.3f, %0.3f, %0.3f], Position [%0.3f, %0.3f, %0.3f]\n", GetEntityId(), clientPos.x, clientPos.y, clientPos.z, serverPos.x, serverPos.y, serverPos.z);
//}
