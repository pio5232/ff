#pragma once

class StatComponent : public BaseComponent
{
public:
	StatComponent();
	~StatComponent();

	bool IsDead() const
	{
		return _curHp == 0;
	}

	void TakeDamage(USHORT damage);

	USHORT GetHp() const { return _curHp; }
	USHORT GetAttackDamage() const { return _attackDamage; }
	float GetAttackRange() const { return _attackRange; }
private:

	USHORT _curHp;
	USHORT _maxHp;

	USHORT _attackDamage;
	float _attackRange;
};