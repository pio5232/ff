#include "pch.h"
#include "SectorManager.h"
#include "Entity.h"
#include "UserManager.h"
#include "GamePlayer.h"
#include "PacketBuilder.h"
#include "Memory.h"

bool jh_content::SectorManager::AddEntity(int sectorZ, int sectorX, EntityPtr entity)
{
	if (false == IsValidSector(sectorX, sectorZ) || false == m_sectorSet[sectorZ][sectorX].insert(entity).second)
	{
		return false;
	}

	if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		m_usAliveGamePlayerCount[sectorZ][sectorX]++;

	return true;
}

bool jh_content::SectorManager::DeleteEntity(EntityPtr entity, PacketBufferRef& sendBuffer)
{
	Sector sector = entity->GetCurrentSector();

	if (false == IsValidSector(sector.m_iX, sector.m_iZ) || m_sectorSet[sector.m_iZ][sector.m_iX].erase(entity) == 0)
	{
		printf("DeleteEntity False\n");
		return false;
	}
	if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		m_usAliveGamePlayerCount[sector.m_iZ][sector.m_iX]--;

	SendPacketAroundSector(sector, sendBuffer);

	return true;
}

void jh_content::SectorManager::SendAllEntityInfo()
{
	std::vector<EntityPtr> aroundEntityInfos;

	jh_network::MakeOtherCharacterPacket makeOtherCharacterPacket;

	// 각 섹터마다 정보를 모아서 주변의 섹터에게 전달.
	for (int z = startZSectorPos; z < endZSectorPos; z++)
	{
		for (int x = startXSectorPos; x < endXSectorPos; x++)
		{
			if (m_sectorSet[z][x].size() == 0)
				continue;

			aroundEntityInfos.clear();

			for (const auto& entity : m_sectorSet[z][x])
			{
				aroundEntityInfos.push_back(entity);
			}

			PacketBufferRef sendBuffer = jh_memory::MakeShared<PacketBuffer>(sizeof(jh_network::MakeOtherCharacterPacket) * aroundEntityInfos.size());

			for (const EntityPtr& entity : aroundEntityInfos)
			{
				*sendBuffer << makeOtherCharacterPacket.size << makeOtherCharacterPacket.type
					<< entity->GetEntityId() << entity->GetPosition();
			}

			SendPacketAroundSector(x, z, sendBuffer);
		}
	}
}
void jh_content::SectorManager::UpdateSector(EntityPtr entity)
{
	AroundSector removeAroundSector, addAroundSector;

	GetUpdatedSectorAround(entity, &removeAroundSector, &addAroundSector);

	// Player가 아니어도 작동해야함.
	// 1. 삭제되어야할 섹터에 존재하는 플레이어에게 삭제 패킷을 보낸다. (나를 삭제해라!!)
	{
		PacketBufferRef delOtherCharacterPkt = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(entity->GetEntityId());

		for (int i = 0; i < removeAroundSector.sectorCount; i++)
		{
			SendPacket_Sector(removeAroundSector.sectors[i], delOtherCharacterPkt);
		}
	}

	// Player가 아니더라도 작동해야한다.
	// 3. 추가되어야할 섹터에 나를 생성 패킷을 보낸다. (나를 만들어라!!!)
	{
		PacketBufferRef makeOtherCharacterPkt = jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(entity->GetEntityId(), entity->GetPosition());
		PacketBufferRef moveStartNotifyPkt = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(entity->GetEntityId(), entity->GetPosition(), entity->GetRotation().y);

		for (int i = 0; i < addAroundSector.sectorCount; i++)
		{
			SendPacket_Sector(addAroundSector.sectors[i], makeOtherCharacterPkt);

			// 만약 내가 이동중이면 이동중인 것까지 보내야한다.
			if (entity->IsMoving())
			{
				SendPacket_Sector(addAroundSector.sectors[i], moveStartNotifyPkt);
			}
		}
	}

	if (entity->GetType() != jh_content::Entity::EntityType::GamePlayer)
		return;

	GamePlayerPtr gamePlayerPtr = std::static_pointer_cast<GamePlayer>(entity);

	ULONGLONG entityId = gamePlayerPtr->GetEntityId();
	// Player일 때만 작동.
	// 2. 내가 바라보는.. (Player) 삭제되어야하는 섹터에 존재하는 entity들을 삭제.
	// entity가 플레이어가 아니라면 보낼 필요 없다. (내가 삭제하겠어!!!)
	for (int i = 0; i < removeAroundSector.sectorCount; i++)
	{
		for (const auto& removeEntity : m_sectorSet[removeAroundSector.sectors[i].m_iZ][removeAroundSector.sectors[i].m_iX])
		{
			PacketBufferRef delOtherCharacterPkt = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(removeEntity->GetEntityId());

			m_sendPacketFunc(entityId, delOtherCharacterPkt);
		}
	}

	// Player일 때만 작동
	// 4. 추가되어야할 섹터의 정보들을 나에게 쏜다. (내가 만들겠다!!!!)
	for (int i = 0; i < addAroundSector.sectorCount; i++)
	{
		for (const auto& addEntity : m_sectorSet[addAroundSector.sectors[i].m_iZ][addAroundSector.sectors[i].m_iX])
		{
			if (addEntity->IsDead())
				continue;

			PacketBufferRef makeOtherCharacterPkt = jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(addEntity->GetEntityId(), addEntity->GetPosition());

			m_sendPacketFunc(entityId, makeOtherCharacterPkt);

			if (addEntity->IsMoving())
			{
				PacketBufferRef moveStartNotifyPkt = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(addEntity->GetEntityId(), addEntity->GetPosition(), addEntity->GetRotation().y);

				m_sendPacketFunc(entityId, moveStartNotifyPkt);
			}
		}
	}
}

void jh_content::SectorManager::SendPacket_Sector(const Sector& sector, PacketBufferRef& packet)
{
	// 유효한 섹터이면서 보낼 대상이 존재해야한다.
	if (false == IsValidSector(sector.m_iX, sector.m_iZ) || 0 == m_usAliveGamePlayerCount[sector.m_iZ][sector.m_iX])
			return;
	
	for (const auto& entity : m_sectorSet[sector.m_iZ][sector.m_iX])
	{
		if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		{
			ULONGLONG entityId = entity->GetEntityId();
			
			m_sendPacketFunc(entityId, packet);
		}
	}
}

void jh_content::SectorManager::SendPacketAroundSector(const Sector& sector, PacketBufferRef& packet)
{
	if (false == IsValidSector(sector.m_iX, sector.m_iZ))
		return;

	AroundSector aroundSector;

	GetSectorAround(sector.m_iX, sector.m_iZ, &aroundSector);

	for (int i = 0; i < aroundSector.sectorCount; i++)
	{
		SendPacket_Sector(aroundSector.sectors[i], packet);
	}
}

void jh_content::SectorManager::SendPacketAroundSector(int sectorX, int sectorZ, PacketBufferRef& packet)
{
	if (false == IsValidSector(sectorX, sectorZ))
		return;

	AroundSector aroundSector;

	GetSectorAround(sectorX, sectorZ, &aroundSector);

	for (int i = 0; i < aroundSector.sectorCount; i++)
	{
		SendPacket_Sector(aroundSector.sectors[i], packet);
	}
}

EntityPtr jh_content::SectorManager::GetMinEntityInRange(EntityPtr targetEntity, float range)
{
	float minRange = range * range;
	EntityPtr minEntity = nullptr;

	// 1. 내 주변 섹터 얻어오기.
	Sector sector = targetEntity->GetCurrentSector();
	AroundSector aroundSector;
	GetSectorAround(sector.m_iX, sector.m_iZ, &aroundSector);

	// 내가 보는 방향
	Vector3 from = targetEntity->GetNormalizedForward();
	Vector3 targetPosition = targetEntity->GetPosition();

	// 2. 얻어온 섹터를 순회하면서 비교한다. 
	// 이 Dummy가 많아지면 이 부분에서 문제가 생길 수 있음.
	// Update를 돌 때.. 모든 살아있는 Entity의 공격 범위에 있는 Entity들을 넣어서 그걸 이용..
	// 캐싱을 하면 중복에 대한 처리가 조금은 될 수 있다.
	for (int i = 0; i < aroundSector.sectorCount; i++)
	{
		for (auto& entity : m_sectorSet[aroundSector.sectors[i].m_iZ][aroundSector.sectors[i].m_iX])
		{
			if (entity->IsDead() || entity == targetEntity)
				continue;

			Vector3 dist = entity->GetPosition() - targetPosition;
			float sqrDist = dist.sqrMagnitude();
			if (sqrDist < minRange)
			{
				Vector3 to = dist.Normalized();

				if (Vector3::Angle(from, to) < 30.0f)
				{
					minRange = sqrDist;
					minEntity = entity;
				}
			}
		}
	}

	return minEntity;
}

void jh_content::SectorManager::Clear()
{
	for (int z = startZSectorPos; z < endZSectorPos; z++)
	{
		for (int x = startXSectorPos; x < endXSectorPos; x++)
		{
			m_sectorSet[z][x].clear();
			m_usAliveGamePlayerCount[z][x] = 0;
		}
	}
}

bool jh_content::SectorManager::IsValidSector(int sectorXPos, int sectorZPos)
{
	return sectorXPos >= startXSectorPos && sectorXPos < endXSectorPos &&
		sectorZPos >= startZSectorPos && sectorZPos < endZSectorPos;
}

void jh_content::SectorManager::GetSectorAround(int sectorXPos, int sectorZPos, AroundSector* pAroundSector)
{
	// 좌표 기준 9칸 
	// [] [] []
	// [] [] []
	// [] [] []

	memset(pAroundSector, 0, sizeof(AroundSector));
	// 테두리를 제외한 부분에 속해있으면 Sector 추가.
	int sectorIndex = 0;
	for (int dz = -1; dz <= 1; dz++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			if (IsValidSector(sectorXPos + dx, sectorZPos + dz))
			{
				pAroundSector->sectors[sectorIndex].m_iX = sectorXPos + dx;
				pAroundSector->sectors[sectorIndex].m_iZ = sectorZPos + dz;

				sectorIndex++;
			}
		}
	}
	pAroundSector->sectorCount = sectorIndex;
}

void jh_content::SectorManager::GetUpdatedSectorAround(EntityPtr entity, AroundSector* pRemoveAroundSector, AroundSector* pAddAroundSector)
{
	int prevSectorX = entity->GetPrevSector().m_iX;
	int prevSectorZ = entity->GetPrevSector().m_iZ;

	int curSectorX = entity->GetCurrentSector().m_iX;
	int curSectorZ = entity->GetCurrentSector().m_iZ;

	// 이전 섹터 9방향 -> 다음 섹터 9방향으로 정보 업데이트
	GetSectorAround(prevSectorX, prevSectorZ, pRemoveAroundSector);
	GetSectorAround(curSectorX, curSectorZ, pAddAroundSector);

	std::set<Sector> addSet;
	std::set<Sector> removeSet;

	for (int i = 0; i < pRemoveAroundSector->sectorCount; i++)
	{
		removeSet.insert({ pRemoveAroundSector->sectors[i].m_iX, pRemoveAroundSector->sectors[i].m_iZ });
	}

	for (int i = 0; i < pAddAroundSector->sectorCount; i++)
	{
		addSet.insert({ pAddAroundSector->sectors[i].m_iX, pAddAroundSector->sectors[i].m_iZ });
	}

	std::vector<Sector> pureAddVec;
	std::vector<Sector> pureRemoveVec;

	for (const auto& sector : addSet)
	{
		if (removeSet.find(sector) == removeSet.end())
			pureAddVec.push_back(sector);
	}

	for (const auto& sector : removeSet)
	{
		if (addSet.find(sector) == addSet.end())
			pureRemoveVec.push_back(sector);
	}

	memset(pRemoveAroundSector, 0, sizeof(AroundSector));
	memset(pAddAroundSector, 0, sizeof(AroundSector));

	// 겹치는 부분을 제외한 새롭게 얻는 부분 / 삭제할 부분에 대한 정보를 얻음

	int idx = 0;
	for (const Sector& sector : pureAddVec)
	{
		pAddAroundSector->sectors[idx++] = sector;
	}
	pAddAroundSector->sectorCount = idx;

	idx = 0;
	for (const Sector& sector : pureRemoveVec)
	{
		pRemoveAroundSector->sectors[idx++] = sector;
	}
	pRemoveAroundSector->sectorCount = idx;

	if (m_sectorSet[prevSectorZ][prevSectorX].erase(entity) == 0)
	{
		_LOG(L"GameWorld", LOG_LEVEL_WARNING, L"[SectorManager::GetUpdatedSectorAround] prev-Sector Erase entity failed. EntityId : [%llu]", entity->GetEntityId());
	}

	if (m_sectorSet[curSectorZ][curSectorX].insert(entity).second == false)
	{
		_LOG(L"GameWorld", LOG_LEVEL_WARNING, L"[SectorManager::GetUpdatedSectorAround] cur-Sector Insert entity failed. EntityId : [%llu]", entity->GetEntityId());
	}

	if (entity->GetType() == Entity::EntityType::GamePlayer)
	{
		m_usAliveGamePlayerCount[prevSectorZ][prevSectorX]--;
		m_usAliveGamePlayerCount[curSectorZ][curSectorX]++;
	}
}
