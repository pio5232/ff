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
	// hp가 uint16이기 때문에 uint16은 0이하로 떨어지지 않음. 그래서 체크를 다르게 하자.

	if (_curHp <= damage)
		_curHp = 0;
	else
		_curHp -= damage;
}
