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

		// ������ ���� ó���� �̱� ������, Room�� weak_ptr<User>�� �����ϴϱ� 
		// ��Ŷ ó�� �ܰ迡�� UserPtr�� ��ȯ�ص� �������.
		UserPtr GetUserByUserId(ULONGLONG userId);
		
	private:

		UserPtr GetUserBySessionId(ULONGLONG sessionId);
	
		// sessionID to userID
		std::unordered_map<ULONGLONG, UserPtr> m_sessionIdToUserUMap;
		
		// userID to userPtr
		std::unordered_map<ULONGLONG, UserPtr> m_userIdToUserUMap;
	};
}