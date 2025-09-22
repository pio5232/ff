#pragma once

class StatComponent : public BaseComponent
{
public:
	StatComponent();
	~StatComponent();

	bool IsDead() const
	{
		return m_usCurHp == 0;
	}

	void TakeDamage(USHORT damage);

	USHORT GetHp() const { return m_usCurHp; }
	USHORT GetAttackDamage() const { return m_usAttackDamage; }
	float GetAttackRange() const { return m_fAttackRange; }
private:

	USHORT m_usCurHp;
	USHORT m_usMaxHp;

	USHORT m_usAttackDamage;
	float m_fAttackRange;
};