#include "pch.h"
#include <Windows.h>
#include "GameWorld.h"
#include "Entity.h"
#include "WorldChat.h"
#include "UserManager.h"
#include "AIPlayer.h"
#include "BufferMaker.h"
#include "SectorManager.h"
#include "PacketBuilder.h"
#include "GamePlayer.h"
#include <algorithm>
#include "GameSession.h"
#include "Memory.h"

jh_content::GameWorld::GameWorld() : m_bIsGameRunning(true), m_bIsUpdateRunning(false), m_fDeltaSum(0)
{
	_sectorManager = std::make_unique<jh_content::SectorManager>();

	srand(GetCurrentThreadId());

	_worldChat = std::make_shared<WorldChat>();
}

jh_content::GameWorld::~GameWorld()
{
	m_aliveEntityDic.clear();
	m_aliveEntityArr.clear();
}

void jh_content::GameWorld::StartGame()
{
	m_bIsUpdateRunning.store(true);

	// 10초마다 초기화
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
	// 네트워크
	if (m_fDeltaSum >= fixedDeltaTime)
	{
		for (EntityPtr& entity : m_aliveEntityArr)
		{
			entity->Update(fixedDeltaTime);

			if (entity->IsSectorUpdated())
			{
				_sectorManager->UpdateSector(entity);
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

	_timerActionQueue.push(std::move(timerAction));

	return true;
}

void jh_content::GameWorld::ProcessTimerActions()
{
	std::vector<TimerAction> timerActions;

	ULONGLONG now = jh_utility::GetTimeStamp();
	{
		while (_timerActionQueue.size() > 0)
		{
			const TimerAction& action = _timerActionQueue.top();

			//printf("[ now : %llu ___ top TimerAction execution Tick : %llu \n",now, action.executeTick);
			if (now < action.executeTick)
				break;

			timerActions.push_back(action);

			_timerActionQueue.pop();
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

void jh_content::GameWorld::AddEntity(EntityPtr entityPtr)
{
	m_aliveEntityDic.insert(std::make_pair(entityPtr->GetEntityId(), entityPtr));
	m_aliveEntityArr.push_back(entityPtr);
	
	int index = m_aliveEntityArr.size() - 1;
	m_aliveEntityToVectorIdxDic.insert(std::make_pair(entityPtr, index));

	Sector sector = entityPtr->GetCurrentSector();
	_sectorManager->AddEntity(sector.z, sector.x, entityPtr);
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
		// entityDic에는 존재하지만, entityToVectorDic에는 존재하지 않는 상황.. 이상한 상황임
		// => insert delete가 동일하게 진행되는지 체크 필요
		printf("Remove Entity - Fatal Error!!!!! Check Please !!!\n");
		return;
	}

	// 2. _entityToVectorIdx.erase
	int idx = entityToVectorIter->second;
	m_aliveEntityToVectorIdxDic.erase(entityToVectorIter);

	int lastIdx = m_aliveEntityArr.size() - 1;

	// _entityToVectorIdx에 저장된 vector index를 이용해 삭제
	// 
	// m_aliveEntityArr[idx]			-------		m_aliveEntityArr[lastIdx]
	//							  ↓
	// m_aliveEntityArr[lastIdx]		-------		m_aliveEntityArr[idx]
	// 탐색하지 않고 마지막 인덱스 원소와 바꿔서 삭제.
	if (lastIdx != idx)
	{
		std::swap(m_aliveEntityArr[idx], m_aliveEntityArr[lastIdx]);

		// m_aliveEntityToVectorIdxDic[ origin lastIdx entity ] = lastIdx;
		//							↓
		// m_aliveEntityToVectorIdxDic[ orign lastIdx entity ] = idx
		m_aliveEntityToVectorIdxDic[m_aliveEntityArr[idx]] = idx;
	}
	// 3. m_aliveEntityArr.erase 
	m_aliveEntityArr.pop_back();

	Sector sector = entityPtr->GetCurrentSector();

	PacketPtr sendBuffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(entityPtr->GetEntityId());

	if (_sectorManager->DeleteEntity(entityPtr, sendBuffer))
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

	_sectorManager->SendAllEntityInfo();

	// 초기화 내용 모두 전송했다는 의미
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
	_sectorManager->SendPacketAroundSector(sector, packet);

	SendToSpectatorEntities(packet);
}

void jh_content::GameWorld::SendPacketAroundSectorNSpectators(int sectorX, int sectorZ, PacketPtr packet)
{
	_sectorManager->SendPacketAroundSector(sectorX, sectorZ, packet);

	SendToSpectatorEntities(packet);

}

void jh_content::GameWorld::CleanUpSpectatorEntities()
{
	auto eraseStartIter = std::remove_if(m_spectatorEntityArr.begin(), m_spectatorEntityArr.end(), [](const std::weak_ptr<jh_content::Entity>& weakEntity) -> bool { return weakEntity.expired(); });

	m_spectatorEntityArr.erase(eraseStartIter, m_spectatorEntityArr.end());

	//// 10초마다 초기화
	TimerAction timerAction;
	timerAction.executeTick = jh_utility::GetTimeStamp() + 10000;

	timerAction.action = [this]() {this->CleanUpSpectatorEntities(); };

	TryEnqueueTimerAction(std::move(timerAction));
}

void jh_content::GameWorld::SendToSpectatorEntities(PacketPtr packet)
{
	// 일정 시간 간격으로 clear해주기 때문에
	// 연결이 끊겼을 경우 참조카운트가 0인 weakPtr 존재할 수 있음.

	for (const std::weak_ptr<jh_content::Entity>& spectatorEntity : m_spectatorEntityArr)
	{
		EntityPtr entityPtr = spectatorEntity.lock();

		if (nullptr == entityPtr)
			continue;

		GameSessionPtr gameSessionPtr = std::static_pointer_cast<GamePlayer>(entityPtr)->GetOwnerSession();

		if (nullptr != gameSessionPtr)
			gameSessionPtr->Send(sendBuffer);
	}
}

void jh_content::GameWorld::SetSpectator(EntityPtr entity)
{
	{
		m_spectatorEntityArr.push_back(entity);

		GameSessionPtr gameSessionPtr = static_pointer_cast<jh_content::GamePlayer>(entity)->GetOwnerSession();

		// 하나씩 보내는 과정을 압축해서 보내게 할 수 있다.
		if (nullptr != gameSessionPtr)
		{
			PacketPtr buffer = PacketBuilder::BuildSpectatorInitPacket();
			gameSessionPtr->Send(buffer);

			for (const EntityPtr& aliveEntityPtr : m_aliveEntityArr)
			{
				if (aliveEntityPtr->IsDead())
					continue;

				PacketPtr makePacketBuffer = PacketBuilder::BuildMakeOtherCharacterPacket(aliveEntityPtr->GetEntityId(), aliveEntityPtr->GetPosition());

				gameSessionPtr->Send(makePacketBuffer);

				if (aliveEntityPtr->IsMoving())
				{
					PacketPtr movePacketBuffer = PacketBuilder::BuildMoveStartNotifyPacket(aliveEntityPtr->GetEntityId(), aliveEntityPtr->GetPosition(), aliveEntityPtr->GetRotation().y);

					gameSessionPtr->Send(movePacketBuffer);
				}
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

	if (m_ullExpectedWinnerId != gamePlayerPtr->GetUserId() &&
		false == wasInVictoryZone && isInVictoryZone)
	{
		// OnEnter
		printf("[Victory Zone Entry... User %llu]\n", gamePlayerPtr->GetUserId());

		m_ullExpectedWinnerId = gamePlayerPtr->GetUserId();
		m_ullExpectedWinTime = jh_utility::GetTimeStamp() + victoryZoneCheckDuration;

		// UpdateWinner Packet 전송.
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildUpdateWinnerNotifyPacket(m_ullExpectedWinnerId, m_ullExpectedWinTime);

		m_pUserManger->Broadcast(sendBuffer);
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
		printf("우승!! 게임 종료 [user id : %llu]!!\n", m_ullExpectedWinnerId);

		// 게임 종료 패킷 전송
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildGameEndNotifyPacket();

		m_pUserMangaer->Broadcast(sendBuffer);
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
	// 무효
	if (userId == m_ullExpectedWinnerId)
	{
		m_ullExpectedWinnerId = 0;
		m_ullExpectedWinTime = MAXULONGLONG;

		printf("[ Invalidate Winner : %llu\n", userId);

		// invalidate winner packet 전송
		PacketPtr sendBuffer = jh_content::PacketBuilder::BuildInvalidateWinnerNotifyPacket(m_ullExpectedWinnerId);

		m_pUserManager->Broadcast(sendBuffer);
		//UserManager::GetInstance().SendToAllPlayer(sendBuffer);
	}
}

void jh_content::GameWorld::HandleAttackPacket(GamePlayerPtr attacker, const jh_network::AttackRequestPacket& packet)
{
	printf("\t\t\tProcessAttackPacket!!!!\n");

	// 1. 공격 상태로 전환 후 모든 녀석들에게 패킷 전송
	attacker->SetAttackState();
	ULONGLONG entityId = attacker->GetEntityId();
	PacketPtr buffer = PacketBuilder::BuildAttackNotifyPacket(entityId);
	SendPacketAroundSectorNSpectators(attacker->GetCurrentSector(), buffer);

	// 2. 데미지 판정. 공격당할 녀석을 얻어온다.
	EntityPtr victimTarget = _sectorManager->GetMinEntityInRange(attacker, attacker->GetAttackRange());
	if (victimTarget == nullptr)
	{
		printf("-------------------------- VicTimTarget is Null -------------------\n");
		return;
	}
	// 3. 체크.
	victimTarget->TakeDamage(attacker->GetAttackDamage());

	if (victimTarget->IsDead())
	{
		PacketPtr buffer = PacketBuilder::BuildDieNotifyPacket(victimTarget->GetEntityId());
		SendPacketAroundSectorNSpectators(victimTarget->GetCurrentSector(), buffer);

		// 죽으면 Update하는 Entity 자료구조에서 제외
		// 죽은 PlayerEntity를 관리하는 자료구조로 옮긴다.

		TimerAction timerAction;

		// 3초뒤 관전자로 전환
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
