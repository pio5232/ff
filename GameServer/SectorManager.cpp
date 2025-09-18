#include "pch.h"
#include "SectorManager.h"
#include "Entity.h"
#include "BufferMaker.h"
#include "UserManager.h"
#include "GamePlayer.h"
#include "PacketBuilder.h"
#include "GameSession.h"


bool jh_content::SectorManager::AddEntity(int sectorZ, int sectorX, EntityPtr entity)
{
	if (false == IsValidSector(sectorX, sectorZ) || false == _sectorSet[sectorZ][sectorX].insert(entity).second)
	{
		return false;
	}

	if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		_aliveGamePlayerCount[sectorZ][sectorX]++;

	return true;
}

bool jh_content::SectorManager::DeleteEntity(EntityPtr entity, jh_network::SharedSendBuffer& sendBuffer)
{
	Sector sector = entity->GetCurrentSector();

	if (false == IsValidSector(sector.x, sector.z) || _sectorSet[sector.z][sector.x].erase(entity) == 0)
	{
		printf("DeleteEntity False\n");
		return false;
	}
	if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		_aliveGamePlayerCount[sector.z][sector.x]--;

	SendPacketAroundSector(sector, sendBuffer);

	/*if (false == IsValidSector(sectorX, sectorZ) || _sectorSet[sectorZ][sectorX].erase(entity) == 0)
	{
		printf("DeleteEntity False\n");
		return false;
	}
	if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		_aliveGamePlayerCount[sectorZ][sectorX]--;

	SendPacketAroundSector(sectorX, sectorZ, sendBuffer);*/

	return true;
}

void jh_content::SectorManager::SendAllEntityInfo()
{
	std::vector<EntityPtr> aroundEntityInfos;

	jh_network::MakeOtherCharacterPacket makeOtherCharacterPacket;

	// �� ���͸��� ������ ��Ƽ� �ֺ��� ���Ϳ��� �Ѹ�.
	for (int z = startZSectorPos; z < endZSectorPos; z++)
	{
		for (int x = startXSectorPos; x < endXSectorPos; x++)
		{
			if (_sectorSet[z][x].size() == 0)
				continue;

			aroundEntityInfos.clear();

			for (const auto& entity : _sectorSet[z][x])
			{
				aroundEntityInfos.push_back(entity);
			}

			jh_network::SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(jh_network::MakeOtherCharacterPacket) * aroundEntityInfos.size());

			for (const EntityPtr& entity : aroundEntityInfos)
			{
				*sendBuffer << makeOtherCharacterPacket.size << makeOtherCharacterPacket.type
					<< entity->GetEntityId() << entity->GetPosition();
			}

			SendPacketAroundSector(x, z, sendBuffer);
		}
	}
}

//void jh_content::SectorManager::SendDeletEntityInfo(EntityPtr delEntity)
//{
//	Sector delEntitySector = delEntity->GetCurrentSector();
//
//	// ����� Player�ۿ� �������� ����.
//
//	jh_network::SharedSendBuffer sendBuffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(delEntity->GetEntityId());
//	for (int dz = -1; dz <= 1; dz++)
//	{
//		for (int dx = -1; dx <= 1; dx++)
//		{
//			if (0 == _aliveGamePlayerCount[delEntitySector.z + dz][delEntitySector.x + dx])
//				continue;
//
//			for (const auto& entity : _sectorSet[delEntitySector.z + dz][delEntitySector.x + dx])
//			{
//				if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
//				{
//					printf("PlayerPosition : [%0.3f, %0.3f, %0.3f], Sector [%u, %u]\n", entity->GetPosition().x, entity->GetPosition().y, entity->GetPosition().z, entity->GetCurrentSector().z, entity->GetCurrentSector().x);
//					ULONGLONG userId = std::static_pointer_cast<GamePlayer>(entity)->GetUserId();
//					jh_content::UserManager::GetInstance().SendToPlayer(sendBuffer, userId);
//				}
//			}
//		}
//	}
//}

void jh_content::SectorManager::UpdateSector(EntityPtr entity)
{
	// ���� player, ai�� �����ϱ� ������ �Ǻ� �� �ؾ��Ѵ�.
	AroundSector removeAroundSector, addAroundSector;

	GetUpdatedSectorAround(entity, &removeAroundSector, &addAroundSector);

	// Player�� �ƴϾ �۵��ؾ���.
	// 1. �����Ǿ���� ���Ϳ� �����ϴ� �÷��̾�� ���� ��Ŷ�� ������. (���� �����ض�!!)
	jh_network::SharedSendBuffer sendBuffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(entity->GetEntityId());

	for (int i = 0; i < removeAroundSector.sectorCount; i++)
	{
		//printf("1_UpdateSector [%d] : %llu [Sector [%d, %d]]\n", i, entity->GetEntityId(),removeAroundSector.sectors[i].z, removeAroundSector.sectors[i].x);
		SendPacket_Sector(removeAroundSector.sectors[i], sendBuffer);
	}



	// Player�� �ƴϴ��� �۵��ؾ��Ѵ�.
	// 3. �߰��Ǿ���� ���Ϳ� ���� ���� ��Ŷ�� ������. (���� ������!!!)
	jh_network::SharedSendBuffer makeBuffer = jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(entity->GetEntityId(), entity->GetPosition());
	jh_network::SharedSendBuffer moveStartBuffer = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(entity->GetEntityId(), entity->GetPosition(), entity->GetRotation().y);

	for (int i = 0; i < addAroundSector.sectorCount; i++)
	{
		//printf("2_UpdateSector [%d] : %llu [Sector [%d, %d]]\n", i, entity->GetEntityId(), removeAroundSector.sectors[i].z, removeAroundSector.sectors[i].x);

		SendPacket_Sector(addAroundSector.sectors[i], makeBuffer);

		// ���� ���� �̵����̸� �̵����� �ͱ��� �������Ѵ�.
		if (entity->IsMoving())
		{
			//printf("2_UpdateSector Move [%d] : %llu [Sector [%d, %d]]\n", i, entity->GetEntityId(), removeAroundSector.sectors[i].z, removeAroundSector.sectors[i].x);
			SendPacket_Sector(addAroundSector.sectors[i], moveStartBuffer);
		}
	}

	if (entity->GetType() != jh_content::Entity::EntityType::GamePlayer)
		return;

	GameSessionPtr gameSessionPtr = std::static_pointer_cast<GamePlayer>(entity)->GetOwnerSession();

	if (nullptr == gameSessionPtr)
	{
		printf("[Update Sector : GameSessionPtr is Nullptr ]\n");
		return;
	}
	// Player�� ���� �۵�.
	// 2. ���� �ٶ󺸴�.. (Player) �����Ǿ���ϴ� ���Ϳ� �����ϴ� entity���� ����.
	// entity�� �÷��̾ �ƴ϶�� ���� �ʿ� ����. (���� �����ϰھ�!!!)
	for (int i = 0; i < removeAroundSector.sectorCount; i++)
	{
		for (const auto& removeEntity : _sectorSet[removeAroundSector.sectors[i].z][removeAroundSector.sectors[i].x])
		{
			jh_network::SharedSendBuffer buffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(removeEntity->GetEntityId());

			//printf("Hey Remove!! Update Sector 3 : %llu\n", entity->GetEntityId());

			gameSessionPtr->Send(buffer);

			//jh_content::UserManager::GetInstance().SendToPlayer(buffer, userId);
		}
	}


	// Player�� ���� �۵�
	// 4. �߰��Ǿ���� ������ �������� ������ ���. (���� ����ڴ�!!!!)
	for (int i = 0; i < addAroundSector.sectorCount; i++)
	{
		for (const auto& addEntity : _sectorSet[addAroundSector.sectors[i].z][addAroundSector.sectors[i].x])
		{
			if (addEntity->IsDead())
				continue;

			jh_network::SharedSendBuffer makeBuffer = jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(addEntity->GetEntityId(), addEntity->GetPosition());

			gameSessionPtr->Send(makeBuffer);

			if (addEntity->IsMoving())
			{
				jh_network::SharedSendBuffer moveStartBuffer = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(addEntity->GetEntityId(), addEntity->GetPosition(), addEntity->GetRotation().y);

				gameSessionPtr->Send(moveStartBuffer);

			}
		}
	}





	//{
	//	// Player�� �ƴϾ �۵��ؾ���.
	//	// 1. �����Ǿ���� ���Ϳ� �����ϴ� �÷��̾�� ���� ��Ŷ�� ������. (���� �����ض�!!)
	//	jh_network::SharedSendBuffer sendBuffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(entity->GetEntityId());
	//	for (int i = 0; i < removeAroundSector.sectorCount; i++)
	//	{
	//		SendPacket_Sector(removeAroundSector.sectors[i], sendBuffer);
	//	}
	//}

	//{
	//	// Player�� ���� �۵�.
	//	// 2. ���� �ٶ󺸴�.. (Player) �����Ǿ���ϴ� ���Ϳ� �����ϴ� entity���� ����.
	//	// entity�� �÷��̾ �ƴ϶�� ���� �ʿ� ����. (���� �����ϰھ�!!!)

	//	if (isGamePlayer)
	//	{
	//		for (int i = 0; i < removeAroundSector.sectorCount; i++)
	//		{
	//			for (const auto& removeEntity : _sectorSet[removeAroundSector.sectors[i].z][removeAroundSector.sectors[i].x])
	//			{
	//				jh_network::SharedSendBuffer sendBuffer = jh_content::PacketBuilder::BuildDeleteOtherCharacterPacket(removeEntity->GetEntityId());

	//				jh_content::UserManager::GetInstance().SendToPlayer(sendBuffer, userId);
	//			}
	//		}
	//	}
	//}

	//{
	//	// Player�� �ƴϴ��� �۵��ؾ��Ѵ�.
	//	// 3. �߰��Ǿ���� ���Ϳ� ���� ���� ��Ŷ�� ������. (���� ������!!!)
	//	jh_network::SharedSendBuffer makeBuffer = jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(entity->GetEntityId(), entity->GetPosition());
	//	jh_network::SharedSendBuffer moveStartBuffer = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(entity->GetEntityId(), entity->GetPosition(), entity->GetRotation().y);
	//	
	//	for (int i = 0; i < addAroundSector.sectorCount; i++)
	//	{
	//		SendPacket_Sector(addAroundSector.sectors[i], makeBuffer);

	//		// ���� ���� �̵����̸� �̵����� �ͱ��� �������Ѵ�.
	//		if (entity->IsMoving())
	//		{
	//			SendPacket_Sector(addAroundSector.sectors[i], moveStartBuffer);
	//		}
	//	}
	//}

	//{
	//	// Player�� ���� �۵�
	//	// 4. �߰��Ǿ���� ������ �������� ������ ���. (���� ����ڴ�!!!!)
	//	for (int i = 0; i < addAroundSector.sectorCount; i++)
	//	{
	//		for (const auto& addEntity : _sectorSet[addAroundSector.sectors[i].z][addAroundSector.sectors[i].x])
	//		{
	//			jh_network::SharedSendBuffer makeBuffer = jh_content::PacketBuilder::BuildMakeOtherCharacterPacket(addEntity->GetEntityId(), addEntity->GetPosition());

	//			jh_content::UserManager::GetInstance().SendToPlayer(makeBuffer, userId);
	//			
	//			if (addEntity->IsMoving())
	//			{
	//				jh_network::SharedSendBuffer moveStartBuffer = jh_content::PacketBuilder::BuildMoveStartNotifyPacket(addEntity->GetEntityId(), addEntity->GetPosition(), addEntity->GetRotation().y);
	//				jh_content::UserManager::GetInstance().SendToPlayer(moveStartBuffer, userId);

	//			}
	//		}
	//	}
	//}
}

void jh_content::SectorManager::SendPacket_Sector(const Sector& sector, jh_network::SharedSendBuffer sendBuffer)
{
	if (false == IsValidSector(sector.x, sector.z) || 0 == _aliveGamePlayerCount[sector.z][sector.x])
	{
		//printf("SendPacket_Sector Return [%d, %d] \n",sector.z,sector.x);
		return;
	}

	for (const auto& entity : _sectorSet[sector.z][sector.x])
	{
		if (entity->GetType() == jh_content::Entity::EntityType::GamePlayer)
		{
			GameSessionPtr gameSessionPtr = std::static_pointer_cast<GamePlayer>(entity)->GetOwnerSession();

			if (nullptr != gameSessionPtr)
				gameSessionPtr->Send(sendBuffer);

			//ULONGLONG userId = std::static_pointer_cast<GamePlayer>(entity)->GetUserId();

			//jh_content::UserManager::GetInstance().SendToPlayer(sendBuffer, userId);
		}
	}
}

void jh_content::SectorManager::SendPacketAroundSector(const Sector& sector, jh_network::SharedSendBuffer sendBuffer)
{
	if (false == IsValidSector(sector.x, sector.z))
		return;

	AroundSector aroundSector;

	GetSectorAround(sector.x, sector.z, &aroundSector);

	//printf("[AroundSector - Sector]GetAroundSectorCount - %d\n", aroundSector.sectorCount);
	//for (int i = 0; i < aroundSector.sectorCount; i++)
	//{
	//	printf("\t\t AroudnSector [%d] = [%d, %d]\n", i, aroundSector.sectors[i].z, aroundSector.sectors[i].x);
	//}

	for (int i = 0; i < aroundSector.sectorCount; i++)
	{
		SendPacket_Sector(aroundSector.sectors[i], sendBuffer);
	}
}

void jh_content::SectorManager::SendPacketAroundSector(int sectorX, int sectorZ, jh_network::SharedSendBuffer sendBuffer)
{
	if (false == IsValidSector(sectorX, sectorZ))
		return;

	AroundSector aroundSector;

	GetSectorAround(sectorX, sectorZ, &aroundSector);

	//printf("[AroundSector - (X,Z)]GetAroundSectorCount - %d\n", aroundSector.sectorCount);
	//for (int i = 0; i < aroundSector.sectorCount; i++)
	//{
	//	printf("\t\t AroudnSector [%d] = [%d, %d]\n", i, aroundSector.sectors[i].z, aroundSector.sectors[i].x);
	//}

	for (int i = 0; i < aroundSector.sectorCount; i++)
	{
		SendPacket_Sector(aroundSector.sectors[i], sendBuffer);
	}
}

void jh_content::SectorManager::PrintSectorInfo() const
{
	//printf("�� ������ �÷��̾� ��, ��Ƽ������ ȯ�濡�� ��Ȯ���� �ʰ� ��µ� �� ����.\n");

	for (int z = startZSectorPos; z < endZSectorPos; z++)
	{
		for (int x = startXSectorPos; x < endXSectorPos; x++)
		{
			printf("[%3hu] ", _aliveGamePlayerCount[z][x]);
		}
		printf("\n");
	}
	printf("\n\n");
}

EntityPtr jh_content::SectorManager::GetMinEntityInRange(EntityPtr targetEntity, float range)
{
	// Range�� ���� �ϳ��� ũ�⺸�� ũ�ٸ�..? ��� ó������
	float minRange = range * range;
	EntityPtr minEntity = nullptr;

	// 1. �� �ֺ� ���� ������.
	Sector sector = targetEntity->GetCurrentSector();
	AroundSector aroundSector;
	GetSectorAround(sector.x, sector.z, &aroundSector);

	// ���� ���� ����
	Vector3 from = targetEntity->GetNormalizedForward();
	Vector3 targetPosition = targetEntity->GetPosition();

	// 2. ���� ���͸� ��ȸ�ϸ鼭 ���Ѵ�. 
	// �� Dummy�� �������� �� �κп��� ������ ���� �� ����.
	// Update�� �� ��.. ��� ����ִ� Entity�� ���� ������ �ִ� Entity���� �־ �װ� �̿�..
	// ĳ���� �ϸ� �ߺ��� ���� ó���� ������ �� �� �ִ�.
	for (int i = 0; i < aroundSector.sectorCount; i++)
	{
		for (auto& entity : _sectorSet[aroundSector.sectors[i].z][aroundSector.sectors[i].x])
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

bool jh_content::SectorManager::IsValidSector(int sectorXPos, int sectorZPos)
{
	return sectorXPos >= startXSectorPos && sectorXPos < endXSectorPos &&
		sectorZPos >= startZSectorPos && sectorZPos < endZSectorPos;
}

void jh_content::SectorManager::GetSectorAround(int sectorXPos, int sectorZPos, AroundSector* pAroundSector)
{
	// ��ǥ ���� 9ĭ 
	// [] [] []
	// [] [] []
	// [] [] []

	memset(pAroundSector, 0, sizeof(AroundSector));
	// �׵θ��� ������ �κп� ���������� Sector �߰�.
	int sectorIndex = 0;
	for (int dz = -1; dz <= 1; dz++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			if (IsValidSector(sectorXPos + dx, sectorZPos + dz))
			{
				pAroundSector->sectors[sectorIndex].x = sectorXPos + dx;
				pAroundSector->sectors[sectorIndex].z = sectorZPos + dz;

				sectorIndex++;
			}
		}
	}
	pAroundSector->sectorCount = sectorIndex;

	//printf("GetSectorAround sector count -> %d\n", sectorIndex);
}

void jh_content::SectorManager::GetUpdatedSectorAround(EntityPtr entity, AroundSector* pRemoveAroundSector, AroundSector* pAddAroundSector)
{
	int prevSectorX = entity->GetPrevSector().x;
	int prevSectorZ = entity->GetPrevSector().z;

	int curSectorX = entity->GetCurrentSector().x;
	int curSectorZ = entity->GetCurrentSector().z;

	//printf("GetUpdatedSector [%d, %d] ---> [%d, %d]\n", prevSectorZ, prevSectorX, curSectorZ, curSectorX);
	// ���� ���� 9����, ���� ���� 9���� get
	GetSectorAround(prevSectorX, prevSectorZ, pRemoveAroundSector);
	GetSectorAround(curSectorX, curSectorZ, pAddAroundSector);

	std::set<Sector> addSet;
	std::set<Sector> removeSet;

	for (int i = 0; i < pRemoveAroundSector->sectorCount; i++)
	{
		removeSet.insert({ pRemoveAroundSector->sectors[i].x, pRemoveAroundSector->sectors[i].z });
	}

	for (int i = 0; i < pAddAroundSector->sectorCount; i++)
	{
		addSet.insert({ pAddAroundSector->sectors[i].x, pAddAroundSector->sectors[i].z });
	}

	std::vector<Sector> pureAddVec;
	std::vector<Sector> pureRemoveVec;

	// 
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

	// ��ġ�� �κ��� ������ ���Ӱ� ��� �κ� / ������ �κп� ���� ������ ����

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

	if (_sectorSet[prevSectorZ][prevSectorX].erase(entity) == 0)
	{
		printf("Sector ���� ���� �α�\n");
	}

	if (_sectorSet[curSectorZ][curSectorX].insert(entity).second == false)
	{
		printf("Sector �߰� ���� �α�\n");
	}

	if (entity->GetType() == Entity::EntityType::GamePlayer)
	{
		_aliveGamePlayerCount[prevSectorZ][prevSectorX]--;
		_aliveGamePlayerCount[curSectorZ][curSectorX]++;
	}
}
