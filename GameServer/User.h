#pragma once

namespace jh_content
{
	class User
	{
	public:
		User(ULONGLONG sessionId, ULONGLONG userId) : m_ullUserId(userId), m_ullSessionId(sessionId), m_pGamePlayer(nullptr)
		{
			InterlockedIncrement64((LONGLONG*)&m_ullAliveLobbyUserCount);
		}

		~User()
		{
			InterlockedDecrement64((LONGLONG*)&m_ullAliveLobbyUserCount);
			
			m_pGamePlayer.reset();
		}

		void SetPlayer(GamePlayerPtr gamePlayer)
		{
			m_pGamePlayer = gamePlayer;
		}

		GamePlayerPtr GetPlayer() { return m_pGamePlayer; }

		ULONGLONG GetUserId() const { return m_ullUserId; }
		static alignas(64) ULONGLONG m_ullAliveLobbyUserCount;
	private:
		const ULONGLONG m_ullSessionId;
		const ULONGLONG m_ullUserId;

		GamePlayerPtr m_pGamePlayer;
	};
}


