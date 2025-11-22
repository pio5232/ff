#pragma once

namespace jh_content
{
	class UserManager
	{
	public:
		friend class LobbySystem;

		UserManager();
		~UserManager();

		void Init();

		UserPtr CreateUser(ULONGLONG sessionId);
		void RemoveUser(ULONGLONG sessionId);

		UserPtr GetUserByUserId(ULONGLONG userId);
		
	private:

		UserPtr GetUserBySessionId(ULONGLONG sessionId);
	
		// sessionID to userID
		std::unordered_map<ULONGLONG, UserPtr> m_sessionIdToUserUMap;
		
		// userID to userPtr
		std::unordered_map<ULONGLONG, UserPtr> m_userIdToUserUMap;
	};
}