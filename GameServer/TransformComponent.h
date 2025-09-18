#pragma once

class TransformComponent : public BaseComponent
{
public:
	TransformComponent(const Vector3& pos);
	TransformComponent();

	~TransformComponent();

	const Vector3 GetRotConst() const { return _rotation; }

	const Vector3& GetPosConst() const { return _position; }

	const Vector3& GetNormalizedDir() const { return _dirNormalized; }
	void SetRandomDirection();

	void Move(float delta);
	void SetDirection(float rotY);
	void SetPosition(const Vector3& pos);
private:
	bool CanGo(float prediction_x, float prediction_z) const;
	bool IsEdge() const;

private:
	Vector3 _rotation;
	Vector3 _position;
	float _moveSpeed;

	Vector3 _dirNormalized;
	//bool _isMoving;
};

