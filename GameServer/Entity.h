#pragma once
#include <iostream>
#include "TransformComponent.h"
#include "Sector.h"
namespace jh_content
{
	class Entity
	{
	public:
		enum class EntityType : byte
		{
			GamePlayer = 0,
			AIPlayer,
		};
	public:
		Entity(EntityType type);
		Entity(EntityType type, const Vector3& startPos);
		~Entity() {}

		virtual void Update(float delta) = 0;
		const Vector3& GetPosition() const { return m_transformComponent.GetPosConst(); }
		const Vector3& GetRotation() const { return m_transformComponent.GetRotConst(); }
		const Vector3& GetNormalizedForward() const { return m_transformComponent.GetNormalizedDir(); }

		ULONGLONG GetEntityId() const { return m_ullEntityId; }
		EntityType GetType() const { return m_entityType; }

		virtual void TakeDamage(USHORT damage) = 0;
		virtual bool IsDead() const = 0;
		virtual USHORT GetHp() const = 0;
		virtual bool IsMoving() const { return false; }

		bool IsSectorUpdated();
		const Sector& GetCurrentSector() const { return m_curSector; }
		const Sector& GetPrevSector() const { return m_prevSector; }
	private:
		ULONGLONG m_ullEntityId;
		const EntityType m_entityType;
	protected:
		TransformComponent m_transformComponent;
		Sector m_curSector;
		Sector m_prevSector;
	};
}