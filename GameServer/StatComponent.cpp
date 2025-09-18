#include "pch.h"
#include "StatComponent.h"

StatComponent::StatComponent() : BaseComponent(ComponentType::Stat),
_curHp(defaultMaxHp), _maxHp(defaultMaxHp), _attackRange(defaultAttackRange), _attackDamage(defaultAttackDamage)
{

}

StatComponent::~StatComponent()
{
}

void StatComponent::TakeDamage(uint16 damage)
{
	// hp�� uint16�̱� ������ uint16�� 0���Ϸ� �������� ����. �׷��� üũ�� �ٸ��� ����.

	if (_curHp <= damage)
		_curHp = 0;
	else
		_curHp -= damage;
}
