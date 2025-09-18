#pragma once

namespace jh_content
{
	class GameSystem;

	class GameLanClient 
	{
	public:
		LanClient(const NetAddress& targetNetAddr, SessionCreator creator);
		~LanClient();

		void SetGameSystem(class GameSystem* gameSystem) { m_pGameSystem = gameSystem; }


	private:
		class GameSystem* m_pGameSystem;
	};
}
