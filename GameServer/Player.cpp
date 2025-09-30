#include "pch.h"
#include "Player.h"
#include "UserManager.h"
#include "PlayerStateController.h"
#include "PacketBuilder.h"
#include "GameWorld.h"
#include "PlayerState.h"
using namespace jh_network;
jh_content::Player::Player(GameWorld* worldPtr, EntityType type, float updateInterval) : Entity(type), m_pWorldPtr(worldPtr), m_lastUpdatePos{}, m_fPosUpdateInterval(updateInterval), m_statComponent()
{
	m_pStateController = std::make_unique<PlayerStateController>(this);

	const Vector3 pos = GetPosition();
	printf("Player Constructor - [ID : %llu, Position : [%0.3f, %0.3f, %0.3f]\n", GetEntityId(), pos.x, pos.y, pos.z);
}

jh_content::Player::~Player()
{

}

void jh_content::Player::Update(float delta)
{

	m_pStateController->Update(delta);
}

void jh_content::Player::Move(float delta)
{
	m_fPosUpdateInterval -= delta;

	if (m_fPosUpdateInterval <= 0)
	{
		m_fPosUpdateInterval = posUpdateInterval;

		SendPositionUpdate();
	}

	m_transformComponent.Move(delta);

	//printf(" Transform Update -  EntityID : %llu, pos [ %0.3f, %0.3f, %0.3f ]]\n",GetEntityId(), m_transformComponent.GetPosConst().m_iX, m_transformComponent.GetPosConst().y, m_transformComponent.GetPosConst().m_iZ);
}

bool jh_content::Player::IsMoving() const
{
	return m_pStateController->GetMoveType() == jh_content::PlayerMoveStateBase::MoveState::Move;
}

void jh_content::Player::TakeDamage(USHORT damage)
{
	m_statComponent.TakeDamage(damage);

	m_pStateController->ChangeState(&jh_content::PlayerAttackedState::GetInstance());
}


void jh_content::Player::BroadcastMoveState()
{
	if (m_pStateController->GetMoveType() == jh_content::PlayerMoveStateBase::MoveState::Move)
	{
		PacketPtr buffer = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(GetEntityId(), m_transformComponent.GetPosConst(), m_transformComponent.GetRotConst().y);

		m_pWorldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), buffer);

		//jh_content::UserManager::GetInstance().SendToAllPlayer(buffer);
	}
	else if (m_pStateController->GetMoveType() == jh_content::PlayerMoveStateBase::MoveState::Idle)
	{
		PacketPtr buffer = jh_content::PacketBuilder::BuildMoveStopNotifyPacket(GetEntityId(), m_transformComponent.GetPosConst(), m_transformComponent.GetRotConst().y);

		m_pWorldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), buffer);

		//jh_content::UserManager::GetInstance().SendToAllPlayer(buffer);
	}
}
void jh_content::Player::SendPositionUpdate()
{
	Vector3 currentPos = m_transformComponent.GetPosConst();

	//if ((currentPos - m_lastUpdatePos).sqrMagnitude() < 0.01f)
	//	return;
	
	if ((currentPos - m_lastUpdatePos).sqrMagnitude() < 0.01f)
		return;

	PacketPtr buffer = jh_content::PacketBuilder::BuildUpdateTransformPacket(jh_utility::GetTimeStamp(), GetEntityId(), currentPos, m_transformComponent.GetRotConst());

	m_pWorldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), buffer);

	m_lastUpdatePos = currentPos;

	printf("SendPositionUpdate\n");
}
