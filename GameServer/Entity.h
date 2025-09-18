#pragma once
#include <iostream>
#include "TransformComponent.h"
#include "Sector.h"
namespace jh_content
{
	class Entity : public std::enable_shared_from_this<Entity>
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
		const Vector3& GetPosition() const { return _transformComponent.GetPosConst(); }
		const Vector3& GetRotation() const { return _transformComponent.GetRotConst(); }
		const Vector3& GetNormalizedForward() const { return _transformComponent.GetNormalizedDir(); }

		ULONGLONG GetEntityId() const { return _entityId; }
		EntityType GetType() const { return _entityType; }

		virtual void TakeDamage(USHORT damage) = 0;
		virtual bool IsDead() const = 0;
		virtual USHORT GetHp() const = 0;
		virtual bool IsMoving() const { return false; }

		bool IsSectorUpdated();
		const Sector& GetCurrentSector() const { return _curSector; }
		const Sector& GetPrevSector() const { return _prevSector; }
	private:
		ULONGLONG _entityId;
		const EntityType _entityType;
	protected:
		TransformComponent _transformComponent;
		Sector _curSector;
		Sector _prevSector;
	};
}