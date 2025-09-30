#include "pch.h"
#include "Entity.h"
#include <random>
jh_content::Entity::Entity(EntityType type) : m_entityType(type), m_transformComponent()
{
	static std::mt19937_64 gen(1);
	static volatile ULONGLONG generator = gen();

	m_ullEntityId = InterlockedIncrement64((LONGLONG*)&generator);

	printf("My ID : %llu\n", m_ullEntityId);

	const Vector3& position = m_transformComponent.GetPosConst();
	m_curSector.m_iX = position.x / sectorCriteriaSize + 1;
	m_curSector.m_iZ = position.z / sectorCriteriaSize + 1;

	m_prevSector = m_curSector;
}

jh_content::Entity::Entity(EntityType type, const Vector3& startPos) : m_entityType(type), m_transformComponent(startPos)
{
	static std::mt19937_64 gen(1);
	static volatile ULONGLONG generator = gen();

	m_ullEntityId = InterlockedIncrement64((LONGLONG*)&generator);

	printf("My ID : %llu\n", m_ullEntityId);

	const Vector3& position = m_transformComponent.GetPosConst();
	m_curSector.m_iX = position.x / sectorCriteriaSize + 1;
	m_curSector.m_iZ = position.z / sectorCriteriaSize + 1;

	m_prevSector = m_curSector;
}

bool jh_content::Entity::IsSectorUpdated()
{
	m_prevSector = m_curSector;

	const Vector3& position = m_transformComponent.GetPosConst();

	m_curSector.m_iX = position.x / sectorCriteriaSize + 1;
	m_curSector.m_iZ = position.z / sectorCriteriaSize + 1;

	return m_prevSector != m_curSector;
}
