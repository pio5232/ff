#pragma once

enum class ComponentType
{
	NONE,
	Transform,
	Stat,
};

class BaseComponent
{
public:
	BaseComponent(ComponentType componentType);
	virtual ~BaseComponent();

	ComponentType GetType() const { return m_componentType; }
private:
	ComponentType m_componentType = ComponentType::NONE;
};

