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

	ComponentType GetType() const { return _componentType; }
private:
	ComponentType _componentType = ComponentType::NONE;
	//ULONGLONG _userId = 0;

};

