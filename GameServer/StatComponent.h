#pragma once

class StatComponent : public BaseComponent
{
public:
	StatComponent();
	~StatComponent();

	void TakeDamage(USHORT damage);

	bool IsDead() const				{ return m_usCurHp == 0; }
	
	USHORT GetHp() const			{ return m_usCurHp; }
	USHORT GetAttackDamage() const	{ return m_usAttackDamage; }
	float GetAttackRange() const	{ return m_fAttackRange; }
private:

	USHORT m_usCurHp;
	USHORT m_usMaxHp;

	USHORT m_usAttackDamage;
	float m_fAttackRange;
};