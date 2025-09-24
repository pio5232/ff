#pragma once

namespace jh_content
{
	class User
	{
	public:
		User(ULONGLONG sessionId, ULONGLONG userId) : m_ullUserId(userId), m_ullSessionId(sessionId), m_pGamePlayer(nullptr)
		{
			m_aliveLobbyUserCount.fetch_add(1);
		}

		~User()
		{
			m_aliveLobbyUserCount.fetch_sub(1);

			m_pGamePlayer.reset();
		}

		void SetPlayer(GamePlayerPtr gamePlayer)
		{
			m_pGamePlayer = gamePlayer;
		}

		GamePlayerPtr GetPlayer() { return m_pGamePlayer; }

		ULONGLONG GetUserId() const { return m_ullUserId; }
		ULONGLONG GetSessionId() const { return m_ullSessionId; }
		static alignas(64) std::atomic<int> m_aliveLobbyUserCount;
	private:
		const ULONGLONG m_ullSessionId;
		const ULONGLONG m_ullUserId;

		GamePlayerPtr m_pGamePlayer;
	};
}


