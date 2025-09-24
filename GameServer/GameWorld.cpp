#include "pch.h"
#include <Windows.h>
#include "GameWorld.h"
#include "UserManager.h"
#include "Entity.h"
#include "AIPlayer.h"
#include "SectorManager.h"
#include "PacketBuilder.h"
#include "GamePlayer.h"
#include <algorithm>
#include "Memory.h"
#include "UserManager.h"
#include "User.h"

jh_content::GameWorld::GameWorld(UserManager* userManager, SendPacketFunc sendPacketFunc) : m_bIsGameRunning(true), m_bIsUpdateRunning(false), m_fDeltaSum(0), m_pUserManager(userManager), m_sendPacketFunc(sendPacketFunc)
{
	// entity ID�� ���ؼ� Send�� �����ϰ� �ϴ� �۾��� �����Ѵ�.
	auto sectorSendFunc = [this](ULONGLONG entityId, PacketPtr& packetPtr) {
		UserPtr userPtr = m_pUserManager->GetUserByEntityId(entityId);

		if (nullptr == userPtr)
		{
			_LOG(L"GameWorld", LOG_LEVEL_SYSTEM, L"[SectorManager - sendPacket] - entityID : [%llu]�� �ش��ϴ� ������ �������� �ʽ��ϴ�", entityId);

			return;
		}

		ULONGLONG sessionId = userPtr->GetSessionId();

		m_sendPacketFunc(sessionId, packetPtr);
		};

	m_pSectorManager = std::make_unique<jh_content::SectorManager>(sectorSendFunc);

	srand(GetCurrentThreadId());

}

jh_content::GameWorld::~GameWorld()
{
	m_aliveEntityDic.clear();
	m_aliveEntityArr.clear();
}

void jh_content::GameWorld::StartGame()
{
	m_bIsUpdateRunning.store(true);

	// 10�ʸ��� �ʱ�ȭ
	TimerAction timerAction;
	timerAction.executeTick = jh_utility::GetTimeStamp() + 3000;

	timerAction.action = [this]() {this->CleanUpSpectatorEntities(); };

	TryEnqueueTimerAction(std::move(timerAction));

	CheckWinner();
}

void jh_content::GameWorld::Stop()
{
}

void jh_content::GameWorld::Update(float deltaTime)
{
	if (false == m_bIsUpdateRunning.load())
		return;

	m_fDeltaSum += deltaTime;
	// ��Ʈ��ũ
	if (m_fDeltaSum >= fixedDeltaTime)
	{
		for (EntityPtr& entity : m_aliveEntityArr)
		{
			entity->Update(fixedDeltaTime);

			if (entity->IsSectorUpdated())
			{
				m_pSectorManager->UpdateSector(entity);
			}

			if (Entity::EntityType::GamePlayer == entity->GetType())
			{
				GamePlayerPtr gamePlayer = static_pointer_cast<GamePlayer>(entity);

				CheckVictoryZoneEntry(gamePlayer);
			}
		}

		m_fDeltaSum -= fixedDeltaTime;
	}


	printf("Update Thread Exit...\n");
}


bool jh_content::GameWorld::TryEnqueueTimerAction(TimerAction&& timerAction)
{
	if (false == m_bIsUpdateRunning.load())
	{
		return false;
	}

	m_timerActionQueue.push(std::move(timerAction));

	return true;
}

void jh_content::GameWorld::ProcessTimerActions()
{
	std::vector<TimerAction> timerActions;

	ULONGLONG now = jh_utility::GetTimeStamp();
	{
		while (m_timerActionQueue.size() > 0)
		{
			const TimerAction& action = m_timerActionQueue.top();

			//printf("[ now : %llu ___ top TimerAction execution Tick : %llu \n",now, action.executeTick);
			if (now < action.executeTick)
				break;

			timerActions.push_back(action);

			m_timerActionQueue.pop();
		}
	}

	for (TimerAction& timerAction : timerActions)
	{
		timerAction.action();
	}

	return;
}

void jh_content::GameWorld::CreateAI(GameWorld* worldPtr)
{
	//AIPlayerPtr aiPlayer = //std::make_shared<jh_content::AIPlayer>(worldPtr);
	AIPlayerPtr aiPlayer = MakeShared<AIPlayer>(g_memAllocator, worldPtr);

	AddEntity(aiPlayer);

}

GamePlayerPtr jh_content::GameWorld::CreateGamePlayer()
{
	return GamePlayerPtr();
}

void jh_content::GameWorld::AddEntity(EntityPtr entityPtr)
{
	m_aliveEntityDic.insert(std::make_pair(entityPtr->GetEntityId(), entityPtr));
	m_aliveEntityArr.push_back(entityPtr);
	
	int index = m_aliveEntityArr.size() - 1;
	m_aliveEntityToVectorIdxDic.insert(std::make_pair(entityPtr, index));

	Sector sector = entityPtr->GetCurrentSector();
	m_pSectorManager->AddEntity(sector.m_iZ, sector.m_iX, entityPtr);
}

void jh_content::GameWorld::RemoveEntity(ULONGLONG entityId)
{
	auto entityDiciter = m_aliveEntityDic.find(entityId);

	if (entityDiciter == m_aliveEntityDic.end())
	{
		printf("Remove Entity - Invalid EntityID\n");
		return;
	}

	EntityPtr entityPtr = entityDiciter->second;
	// 1. m_aliveEntityDic.erase
	m_aliveEntityDic.erase(entityDiciter);

	auto entityToVectorIter = m_aliveEntityToVectorIdxDic.find(entityPtr);

	if (entityToVectorIter == m_aliveEntityToVectorIdxDic.end())
	{
		// entityDic���� ����������, entityToVectorDic���� �������� �ʴ� ��Ȳ.. �̻��� ��Ȳ��
		// => insert delete�� �����ϰ� ����Ǵ��� üũ �ʿ�
		printf("Remove Entity - Fatal Error!!!!! Check Please !!!\n");
		return;
	}

	// 2. _entityToVectorIdx.erase
	int idx = entityToVectorIter->second;
	m_aliveEntityToVectorIdxDic.erase(entityToVectorIter);

	int lastIdx = m_aliveEntityArr.size() - 1;

	// _entityToVectorIdx�� ����� vector index�� �̿��� ����
	// 
	// m_aliveEntityArr[idx]			-------		m_aliveEntityArr[lastIdx]
	//							  ��
	// m_aliveEntityArr[lastIdx]		-------		m_aliveEntityArr[idx]
	// Ž������ �ʰ� ������ �ε��� ���ҿ� �ٲ㼭 ����.
	if (lastIdx != idx)
	{
		std::swap(m_aliveEntityArr[idx], m_aliveEntityArr[lastIdx]);

		// m_aliveEntityToVectorIdxDic[ origin lastIdx entity ] = lastIdx;
		//							��
		// m_aliveEntityToVectorIdxDic[ orign lastIdx entity ] = idx
		m_aliveEntityToVectorIdxDic[m_aliveEntityArr[idx]] = idx;
	}
	// 3. m_aliveEntityArr.erase 
	m_aliveEntityArr.pop_back();

	Sector sector = entityPtr->GetCurrentSector();

	PacketPtr sendBuffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(entityPtr->GetEntityId());

	if (m_pSectorManager->DeleteEntity(entityPtr, sendBuffer))
		SendToSpectatorEntities(sendBuffer);
}

void jh_content::GameWorld::Init(USHORT total, USHORT gamePlayerCount)
{
	SetDSCount(gamePlayerCount);

	USHORT aiCount = total - gamePlayerCount;

	for (int i = 0; i < (int)aiCount; i++)
	{
		CreateAI(this);
	}

	m_pSectorManager->SendAllEntityInfo();

	// �ʱ�ȭ ���� ��� �����ߴٴ� �ǹ�
	PacketPtr sendBuffer = jh_content::PacketBuilder::BuildGameInitDonePacket();

	m_pUserManager->Broadcast(sendBuffer);
	//jh_content::UserManager::GetInstance().SendToAllPlayer(sendBuffer);
}

void jh_content::GameWorld::SetDSCount(USHORT predMaxCnt)
{
	m_aliveEntityDic.reserve(predMaxCnt);
	m_aliveEntityArr.reserve(predMaxCnt);
	m_aliveEntityToVectorIdxDic.reserve(predMaxCnt);
}

void jh_content::GameWorld::SendPacketAroundSectorNSpectators(const Sector& sector, PacketPtr packet)
{
	m_pSectorManager->SendPacketAroundSector(sector, packet);

	SendToSpectatorEntities(packet);
}

void jh_content::GameWorld::SendPacketAroundSectorNSpectators(int sectorX, int sectorZ, PacketPtr packet)
{
	m_pSectorManager->SendPacketAroundSector(sectorX, sectorZ, packet);

	SendToSpectatorEntities(packet);

}

void jh_content::GameWorld::CleanUpSpectatorEntities()
{
	auto eraseStartIter = std::remove_if(m_spectatorEntityArr.begin(), m_spectatorEntityArr.end(), [](const std::weak_ptr<jh_content::Entity>& weakEntity) -> bool { return weakEntity.expired(); });

	m_spectatorEntityArr.erase(eraseStartIter, m_spectatorEntityArr.end());

	//// 10�ʸ��� �ʱ�ȭ
	TimerAction timerAction;
	timerAction.executeTick = jh_utility::GetTimeStamp() + 10000;

	timerAction.action = [this]() {this->CleanUpSpectatorEntities(); };

	TryEnqueueTimerAction(std::move(timerAction));
}

void jh_content::GameWorld::SendToSpectatorEntities(PacketPtr& packet)
{
	// ���� �ð� �������� clear���ֱ� ������
	// ������ ������ ��� ����ī��Ʈ�� 0�� weakPtr ������ �� ����.

	for (const std::weak_ptr<jh_content::Entity>& spectatorEntity : m_spectatorEntityArr)
	{
		EntityPtr entityPtr = spectatorEntity.lock();

		if (nullptr == entityPtr)
			continue;

		ULONGLONG entityId = entityPtr->GetEntityId();
		
		SendToEntity(entityId, packet);
	}
}

void jh_content::GameWorld::SetSpectator(EntityPtr entity)
{
	{
		m_spectatorEntityArr.push_back(entity);

		ULONGLONG entityId = entity->GetEntityId();

		PacketPtr spectatorInitPkt = PacketBuilder::BuildSpectatorInitPacket();
		
		SendToEntity(entityId, spectatorInitPkt);

		// ��� ������ ������ �����Ѵ�.
		for (const EntityPtr& aliveEntityPtr : m_aliveEntityArr)
		{
			if (aliveEntityPtr->IsDead())
				continue;

			ULONGLONG aliveEntityId = aliveEntityPtr->GetEntityId();
			const Vector3& aliveEntityPos = aliveEntityPtr->GetPosition();
			const Vector3& aliveEntityRot = aliveEntityPtr->GetRotation();

			PacketPtr makeOtherCharacterPkt = PacketBuilder::BuildMakeOtherCharacterPacket(aliveEntityId, aliveEntityPos);

			SendToEntity(entityId, makeOtherCharacterPkt);

			if (aliveEntityPtr->IsMoving())
			{
				PacketPtr MoveStartNotifyPkt = PacketBuilder::BuildMoveStartNotifyPacket(aliveEntityId, aliveEntityPos, aliveEntityRot.y);

				SendToEntity(entityId, MoveStartNotifyPkt);
			}
		}
		
	}
}

bool jh_content::GameWorld::IsInVictoryZone(const Vector3& pos) const
{
	return pos.x >= VictoryZoneMinX && pos.x <= VictoryZoneMaxX
		&& pos.z >= VictoryZoneMinZ && pos.z <= VictoryZoneMaxZ;
}

void jh_content::GameWorld::CheckVictoryZoneEntry(GamePlayerPtr gamePlayerPtr)
{
	bool wasInVictoryZone = gamePlayerPtr->GetWasInVictoryZone();
	bool isInVictoryZone = IsInVictoryZone(gamePlayerPtr->GetPosition());

	UserPtr userPtr = gamePlayerPtr->GetOwnerUser();

	if (nullptr == userPtr)
	{
		_LOG(L"GameWorld", LOG_LEVEL_SYSTEM, L"[CheckVictoryZoneEntry] - entityID : [%llu]�� �ش��ϴ� ������ �������� �ʽ��ϴ�", gamePlayerPtr->GetEntityId());

		return;
	}

	ULONGLONG userId = userPtr->GetUserId();

	if (m_ullExpectedWinnerId != userId &&
		false == wasInVictoryZone && isInVictoryZone)
	{
		// OnEnter
		printf("[Victory Zone Entry... User %llu]\n", userId);

		m_ullExpectedWinnerId = userId;
		m_ullExpectedWinTime = jh_utility::GetTimeStamp() + victoryZoneCheckDuration;

		// UpdateWinner Packet ����.
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildUpdateWinnerNotifyPacket(m_ullExpectedWinnerId, m_ullExpectedWinTime);

		m_pUserManager->Broadcast(sendBuffer);
		//UserManager::GetInstance().SendToAllPlayer(sendBuffer);
	}

	if (wasInVictoryZone != isInVictoryZone)
		gamePlayerPtr->SetWasInVictoryZone(isInVictoryZone);

}

void jh_content::GameWorld::CheckWinner()
{
	ULONGLONG now = jh_utility::GetTimeStamp();

	if (now >= m_ullExpectedWinTime)
	{
		printf("���!! ���� ���� [user id : %llu]!!\n", m_ullExpectedWinnerId);

		// ���� ���� ��Ŷ ����
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildGameEndNotifyPacket();

		m_pUserManager->Broadcast(sendBuffer);
		//UserManager::GetInstance().SendToAllPlayer(sendBuffer);

		m_bIsGameRunning = false;
		return;
	}

	TimerAction timerAction;
	timerAction.executeTick = now + 2000;

	timerAction.action = [this]() {this->CheckWinner(); };

	TryEnqueueTimerAction(std::move(timerAction));
}

void jh_content::GameWorld::InvalidateWinner(ULONGLONG userId)
{
	printf("InvalidateWinner Enter .. UserId : %llu, _expectedWinner ID : %llu", userId, m_ullExpectedWinnerId);
	// ��ȿ
	if (userId == m_ullExpectedWinnerId)
	{
		m_ullExpectedWinnerId = 0;
		m_ullExpectedWinTime = MAXULONGLONG;

		printf("[ Invalidate Winner : %llu\n", userId);

		// invalidate winner packet ����
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildInvalidateWinnerNotifyPacket(m_ullExpectedWinnerId);

		m_pUserManager->Broadcast(sendBuffer);
		//UserManager::GetInstance().SendToAllPlayer(sendBuffer);
	}
}

void jh_content::GameWorld::ProcessAttack(GamePlayerPtr attacker)
{
	printf("\t\t\tProcessAttackPacket!!!!\n");

	// 1. ���� ���·� ��ȯ �� ��� �༮�鿡�� ��Ŷ ����
	attacker->SetAttackState();
	ULONGLONG entityId = attacker->GetEntityId();
	PacketPtr buffer = PacketBuilder::BuildAttackNotifyPacket(entityId);
	SendPacketAroundSectorNSpectators(attacker->GetCurrentSector(), buffer);

	// 2. ������ ����. ���ݴ��� �༮�� ���´�.
	EntityPtr victimTarget = m_pSectorManager->GetMinEntityInRange(attacker, attacker->GetAttackRange());
	if (victimTarget == nullptr)
	{
		printf("-------------------------- VicTimTarget is Null -------------------\n");
		return;
	}
	// 3. üũ.
	victimTarget->TakeDamage(attacker->GetAttackDamage());

	if (victimTarget->IsDead())
	{
		PacketPtr buffer = PacketBuilder::BuildDieNotifyPacket(victimTarget->GetEntityId());
		SendPacketAroundSectorNSpectators(victimTarget->GetCurrentSector(), buffer);

		// ������ Update�ϴ� Entity �ڷᱸ������ ����
		// ���� PlayerEntity�� �����ϴ� �ڷᱸ���� �ű��.

		TimerAction timerAction;

		// 3�ʵ� �����ڷ� ��ȯ
		std::weak_ptr<Entity> victimTargetWptr = victimTarget;
		timerAction.executeTick = jh_utility::GetTimeStamp() + 3000;
		timerAction.action = [this, victimTargetWptr]() {

			EntityPtr victimTarget = victimTargetWptr.lock();

			if (nullptr == victimTarget)
				return;

			RemoveEntity(victimTarget->GetEntityId());

			if (victimTarget->GetType() == jh_content::Entity::EntityType::GamePlayer)
				SetSpectator(victimTarget);
			};

		TryEnqueueTimerAction(std::move(timerAction));
	}
	else
	{
		PacketPtr buffer = PacketBuilder::BuildAttackedNotifyPacket(victimTarget->GetEntityId(), victimTarget->GetHp());
		SendPacketAroundSectorNSpectators(victimTarget->GetCurrentSector(), buffer);
	}
}
