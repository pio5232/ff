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

		SectorManager(SendPacketFunc sendPacketFunc) : m_sendPacketFunc(sendPacketFunc){}
		~SectorManager() = default;
		bool AddEntity(int sectorZ, int sectorX, EntityPtr entity);

		//bool DeleteEntity(int sectorZ, int sectorX, EntityPtr entity, jh_network::SharedSendBuffer sendBuffer);
		bool DeleteEntity(EntityPtr entity, PacketPtr& sendBuffer);

		void SendAllEntityInfo();

		void UpdateSector(EntityPtr entity);

		// ���� �ϳ��� ��Ŷ ����
		void SendPacket_Sector(const Sector& sector, PacketPtr& packet);

		// �ֺ� ���Ϳ� ��Ŷ ����
		void SendPacketAroundSector(const Sector& sector, PacketPtr& packet);
		void SendPacketAroundSector(int sectorX, int sectorZ, PacketPtr& packet);

		void PrintSectorInfo() const;
		EntityPtr GetMinEntityInRange(EntityPtr targetEntity, float range);

	private:
		bool IsValidSector(int sectorXPos, int sectorZPos);
		// ���� 9���� ������.
		void GetSectorAround(int sectorXPos, int sectorZPos, class AroundSector* pAroundSector);

		// ���Ͱ� ����Ǿ��� �� ���� ������ Ȯ��
		void GetUpdatedSectorAround(EntityPtr entity, class AroundSector* pRemoveAroundSector, class AroundSector* pAddAroundSector);
		// ������ entity��ü�� GameWorld��� ���� Ŭ�������� ����. �̳༮�� sector������ �Ѵ�.
		std::set<EntityPtr> m_sectorSet[sectorMaxZ][sectorMaxX];
		USHORT m_usAliveGamePlayerCount[sectorMaxZ][sectorMaxX];
		
		SendPacketFunc m_sendPacketFunc;
	};
}