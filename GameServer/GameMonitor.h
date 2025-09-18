#pragma once
#include "CMonitor.h"

namespace jh_content
{
	class SectorManager;
}
namespace jh_utility
{
	class GameMonitor : public jh_utility::CMonitor
	{
	public:
		GameMonitor(class jh_network::SessionManager* sessionMgr, const class jh_content::SectorManager* const sectorManager) : _sessionMgr(sessionMgr), _sectorManager(sectorManager) {}
		virtual void MonitoringJob() override;

	private:
		class jh_network::SessionManager* _sessionMgr;
		const class jh_content::SectorManager* const _sectorManager;
	};
}
