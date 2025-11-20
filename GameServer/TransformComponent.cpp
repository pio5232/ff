#include "pch.h"
#include "TransformComponent.h"

TransformComponent::TransformComponent(const Vector3& pos) : BaseComponent(ComponentType::Transform), m_position(pos), m_rotation{ Vector3::Zero() }, m_fMoveSpeed(defaultWalkSpeed), m_dirNormalized(Vector3::Zero())
{
}
TransformComponent::TransformComponent() : BaseComponent(ComponentType::Transform), m_fMoveSpeed(defaultWalkSpeed), m_rotation{ Vector3::Zero() }, m_dirNormalized(Vector3::Zero())
{
	m_position = GenerateRandomPos();
}

TransformComponent::~TransformComponent()
{
}


void TransformComponent::SetRandomDirection()
{
	if (IsEdge() && CheckChance(30))
	{
		Vector3 positionToCenter = Vector3(centerX, 0, centerZ) - m_position;

		float rotY = atan2f(positionToCenter.x, positionToCenter.z) * deg2Rad;

		SetDirection(rotY);

		return;
	}
	const float randomRange = 135.0f; // -135 ~ 135

	float offset = static_cast<float>(GetRandDouble(-randomRange, randomRange));

	SetDirection(m_rotation.y + offset);

	return;
}

void TransformComponent::Move(float delta)
{

	Vector3 moveVector = m_dirNormalized * delta * m_fMoveSpeed;
	if (CanGo(m_position.x + moveVector.x, m_position.z + moveVector.z) == false)
		return;

	m_position += moveVector;
}

void TransformComponent::SetDirection(float rotY)
{
	const float deg2Rad = 3.141592f / 180.0f;

	m_rotation.y = NormalizeAngle(rotY);

	// transform.forward.normalized와 같다. 내가 바라보고 있는 방향의 방향 벡터, 현재 로테이션은 RotY
	m_dirNormalized = Vector3(sinf(m_rotation.y * deg2Rad), 0, cosf(m_rotation.y * deg2Rad));

}

void TransformComponent::SetPosition(const Vector3& pos)
{
	m_position = pos;
}

bool TransformComponent::CanGo(float prediction_x, float prediction_z) const
{
	return prediction_x > mapXMin && prediction_x < mapXMax && prediction_z > mapZMin && prediction_z < mapZMax;
}

bool TransformComponent::IsEdge() const
{
	return m_position.x < edgeThreshold || m_position.x >(mapXMax - edgeThreshold)
		|| m_position.z < edgeThreshold || m_position.z >(mapZMax - edgeThreshold);
}
