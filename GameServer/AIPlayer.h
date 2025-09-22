#pragma once

#include "Player.h"

namespace jh_content
{
	class GameWorld;
	class AIPlayer : public Player
	{
	public:
		AIPlayer(GameWorld* worldPtr);
		~AIPlayer() { printf("~~~AI Destructor\n"); }

		virtual void Update(float delta);

		void UpdateAIMovement();

	private:
		float m_fMovementUpdateInterval;
	};
}