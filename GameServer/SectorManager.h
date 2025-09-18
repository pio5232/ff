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

		// 섹터 하나에 패킷 전송
		void SendPacket_Sector(const Sector& sector, jh_network::SharedSendBuffer sendBuffer);

		// 주변 섹터에 패킷 전송
		void SendPacketAroundSector(const Sector& sector, jh_network::SharedSendBuffer sendBuffer);
		void SendPacketAroundSector(int sectorX, int sectorZ, jh_network::SharedSendBuffer sendBuffer);

		void PrintSectorInfo() const;
		EntityPtr GetMinEntityInRange(EntityPtr targetEntity, float range);

	private:
		bool IsValidSector(int sectorXPos, int sectorZPos);
		// 주위 9섹터 얻어오기.
		void GetSectorAround(int sectorXPos, int sectorZPos, class AroundSector* pAroundSector);

		// 섹터가 변경되었을 때 변경 지점을 확인
		void GetUpdatedSectorAround(EntityPtr entity, class AroundSector* pRemoveAroundSector, class AroundSector* pAddAroundSector);
		// 어차피 entity자체는 GameWorld라는 상위 클래스에서 관리. 이녀석은 sector관리만 한다.
		std::set<EntityPtr> _sectorSet[sectorMaxZ][sectorMaxX];
		USHORT _aliveGamePlayerCount[sectorMaxZ][sectorMaxX];

	};
}