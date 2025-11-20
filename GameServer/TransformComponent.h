#pragma once

class TransformComponent : public BaseComponent
{
public:
	TransformComponent(const Vector3& pos);
	TransformComponent();

	~TransformComponent();

	void Move(float delta);

	void SetRandomDirection();
	void SetDirection(float rotY);
	void SetPosition(const Vector3& pos);

	const Vector3 GetRotConst() const		{ return m_rotation; }
	const Vector3& GetPosConst() const		{ return m_position; }
	const Vector3& GetNormalizedDir() const { return m_dirNormalized; }
private:
	bool CanGo(float prediction_x, float prediction_z) const;
	bool IsEdge() const;

private:
	Vector3 m_rotation;
	Vector3 m_position;
	float m_fMoveSpeed;

	Vector3 m_dirNormalized;
};

