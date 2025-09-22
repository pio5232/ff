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
	// hp�� uint16�̱� ������ uint16�� 0���Ϸ� �������� ����. �׷��� üũ�� �ٸ��� ����.

	if (m_usCurHp <= damage)
		m_usCurHp = 0;
	else
		m_usCurHp -= damage;
}
