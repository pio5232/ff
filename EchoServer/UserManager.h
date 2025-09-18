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

		// 어차피 로직 처리는 싱글 스레드, Room은 weak_ptr<User>를 저장하니까 
		// 패킷 처리 단계에서 UserPtr로 반환해도 상관없다.
		UserPtr GetUser(ULONGLONG sessionId);

	private:
		// session
		std::unordered_map<ULONGLONG, UserPtr> userInfo;
	};
}