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

		bool DeleteEntity(EntityPtr entity, PacketBufferRef& sendBuffer);

		void SendAllEntityInfo();

		void UpdateSector(EntityPtr entity);

		// 섹터 하나에 패킷 전송
		void SendPacket_Sector(const Sector& sector, PacketBufferRef& packet);

		// 주변 섹터에 패킷 전송
		void SendPacketAroundSector(const Sector& sector, PacketBufferRef& packet);
		void SendPacketAroundSector(int sectorX, int sectorZ, PacketBufferRef& packet);

		EntityPtr GetMinEntityInRange(EntityPtr targetEntity, float range);

		void Clear();

	private:
		bool IsValidSector(int sectorXPos, int sectorZPos);
		// 주위 9섹터 얻어오기.
		void GetSectorAround(int sectorXPos, int sectorZPos, class AroundSector* pAroundSector);

		// 섹터가 변경되었을 때 변경 지점을 확인
		void GetUpdatedSectorAround(EntityPtr entity, class AroundSector* pRemoveAroundSector, class AroundSector* pAddAroundSector);
		
		// entity는 GameWorld라는 상위 클래스에서 관리. 이 클래스는 sector관리만 한다.
		std::set<EntityPtr> m_sectorSet[sectorMaxZ][sectorMaxX];
		USHORT				m_usAliveGamePlayerCount[sectorMaxZ][sectorMaxX];
		
		SendPacketFunc		m_sendPacketFunc;
	};
}