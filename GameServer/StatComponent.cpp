#include "pch.h"
#include "StatComponent.h"

StatComponent::StatComponent() : BaseComponent(ComponentType::Stat),
m_usCurHp(defaultMaxHp), m_usMaxHp(defaultMaxHp), m_fAttackRange(defaultAttackRange), m_usAttackDamage(defaultAttackDamage)
{

}

StatComponent::~StatComponent()
{
}

void StatComponent::TakeDamage(USHORT damage)
{
	// hp가 uint16이기 때문에 uint16은 0이하로 떨어지지 않음. 그래서 체크를 다르게 하자.

	if (m_usCurHp <= damage)
		m_usCurHp = 0;
	else
		m_usCurHp -= damage;
}
