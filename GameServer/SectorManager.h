#pragma once
#include <array>>
#include <set>
#include <functional>
#include "Sector.h"
namespace jh_content
{
	class SectorManager
	{
	public:
		bool AddEntity(int sectorZ, int sectorX, EntityPtr entity);

		//bool DeleteEntity(int sectorZ, int sectorX, EntityPtr entity, jh_network::SharedSendBuffer sendBuffer);
		bool DeleteEntity(EntityPtr entity, jh_network::SharedSendBuffer& sendBuffer);

		void SendAllEntityInfo();

		void UpdateSector(EntityPtr entity);

		// ���� �ϳ��� ��Ŷ ����
		void SendPacket_Sector(const Sector& sector, jh_network::SharedSendBuffer sendBuffer);

		// �ֺ� ���Ϳ� ��Ŷ ����
		void SendPacketAroundSector(const Sector& sector, jh_network::SharedSendBuffer sendBuffer);
		void SendPacketAroundSector(int sectorX, int sectorZ, jh_network::SharedSendBuffer sendBuffer);

		void PrintSectorInfo() const;
		EntityPtr GetMinEntityInRange(EntityPtr targetEntity, float range);

	private:
		bool IsValidSector(int sectorXPos, int sectorZPos);
		// ���� 9���� ������.
		void GetSectorAround(int sectorXPos, int sectorZPos, class AroundSector* pAroundSector);

		// ���Ͱ� ����Ǿ��� �� ���� ������ Ȯ��
		void GetUpdatedSectorAround(EntityPtr entity, class AroundSector* pRemoveAroundSector, class AroundSector* pAddAroundSector);
		// ������ entity��ü�� GameWorld��� ���� Ŭ�������� ����. �̳༮�� sector������ �Ѵ�.
		std::set<EntityPtr> _sectorSet[sectorMaxZ][sectorMaxX];
		USHORT _aliveGamePlayerCount[sectorMaxZ][sectorMaxX];

	};
}