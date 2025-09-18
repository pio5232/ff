#pragma once

namespace jh_content
{
	class UserManager
	{
	public:
		UserManager();
		~UserManager();

		void Init();

		UserPtr CreateUser(ULONGLONG sessionId);
		void RemoveUser(ULONGLONG sessionId);

		// ������ ���� ó���� �̱� ������, Room�� weak_ptr<User>�� �����ϴϱ� 
		// ��Ŷ ó�� �ܰ迡�� UserPtr�� ��ȯ�ص� �������.
		UserPtr GetUser(ULONGLONG sessionId);

	private:
		// session
		std::unordered_map<ULONGLONG, UserPtr> userInfo;
	};
}