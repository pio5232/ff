#include "pch.h"
#include "Player.h"
#include "UserManager.h"
#include "BufferMaker.h"
#include "PlayerStateController.h"
#include "PacketBuilder.h"
#include "GameWorld.h"
#include "PlayerState.h"
using namespace jh_network;
jh_content::Player::Player(GameWorld* worldPtr, EntityType type, float updateInterval) : Entity(type), _worldPtr(worldPtr), _lastUpdatePos{}, _posUpdateInterval(updateInterval), _statComponent()
{
	_stateController = std::make_unique<PlayerStateController>(this);

	const Vector3 pos = GetPosition();
	printf("Player Constructor - [ID : %llu, Position : [%0.3f, %0.3f, %0.3f]\n", GetEntityId(), pos.x, pos.y, pos.z);
}

jh_content::Player::~Player()
{

}

void jh_content::Player::Update(float delta)
{

	_stateController->Update(delta);
}

void jh_content::Player::Move(float delta)
{
	//_posUpdateInterval -= delta;

	//if (_posUpdateInterval <= 0)
	//{
	//	_posUpdateInterval = posUpdateInterval;

	SendPositionUpdate();
	//}

	_transformComponent.Move(delta);

	//printf(" Transform Update -  EntityID : %llu, pos [ %0.3f, %0.3f, %0.3f ]]\n",GetEntityId(), _transformComponent.GetPosConst().x, _transformComponent.GetPosConst().y, _transformComponent.GetPosConst().z);
}

bool jh_content::Player::IsMoving() const
{
	return _stateController->GetMoveType() == jh_content::PlayerMoveStateBase::MoveState::Move;
}

void jh_content::Player::TakeDamage(uint16 damage)
{
	_statComponent.TakeDamage(damage);

	_stateController->ChangeState(&jh_content::PlayerAttackedState::GetInstance());
}


void jh_content::Player::BroadcastMoveState()
{
	if (_stateController->GetMoveType() == jh_content::PlayerMoveStateBase::MoveState::Move)
	{
		jh_network::SharedSendBuffer buffer = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(GetEntityId(), _transformComponent.GetPosConst(), _transformComponent.GetRotConst().y);

		_worldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), buffer);

		//jh_content::UserManager::GetInstance().SendToAllPlayer(buffer);
	}
	else if (_stateController->GetMoveType() == jh_content::PlayerMoveStateBase::MoveState::Idle)
	{
		jh_network::SharedSendBuffer buffer = jh_content::PacketBuilder::BuildMoveStopNotifyPacket(GetEntityId(), _transformComponent.GetPosConst(), _transformComponent.GetRotConst().y);

		_worldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), buffer);

		//jh_content::UserManager::GetInstance().SendToAllPlayer(buffer);
	}
}
void jh_content::Player::SendPositionUpdate()
{
	Vector3 currentPos = _transformComponent.GetPosConst();

	if ((currentPos - _lastUpdatePos).sqrMagnitude() < 0.01f)
		return;

	SharedSendBuffer buffer = jh_content::PacketBuilder::BuildUpdateTransformPacket(jh_utility::GetTimeStamp(), GetEntityId(), currentPos, _transformComponent.GetRotConst());

	//jh_content::UserManager::GetInstance().SendToAllPlayer(buffer);
	_worldPtr->SendPacketAroundSectorNSpectators(GetCurrentSector(), buffer);

	_lastUpdatePos = currentPos;
}
