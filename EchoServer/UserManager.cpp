#include "pch.h"
#include "UserManager.h"
#include "User.h"

/// +---------------------------+
/// |		UserManager			|
/// +---------------------------+
//
//jh_content::UserManager::UserManager()
//{
//}
//
//jh_content::UserManager::~UserManager()
//{
//}
//
//void jh_content::UserManager::Init()
//{
//}
//
//UserPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId)
//{
//	// 현재 로그인 작업이 없기 때문에 이렇게 UserId를 얻어옴.
//	// 이게 아니라면 아마 ID를 얻어와야겠지?
//	static ULONGLONG userIdGenerator = 4283;
//	userIdGenerator += 3;
//
//	UserPtr userPtr = std::make_shared<jh_content::User>(userIdGenerator);
//
//	if (userInfo.find(sessionId) != userInfo.end())
//	{
//		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"CreateUser - Sessionid %llu is Duplicated", sessionId);
//		return nullptr;
//	}
//	userInfo.insert({ sessionId, userPtr });
//
//	return userPtr;
//}
//
//void jh_content::UserManager::RemoveUser(ULONGLONG sessionId)
//{
//	userInfo.erase(sessionId);
//}
//
//UserPtr jh_content::UserManager::GetUser(ULONGLONG sessionId)
//{
//	auto findIter = userInfo.find(sessionId);
//
//	// 해당 세션 User가 존재하는 경우
//	if (findIter == userInfo.end())
//	{
//		return findIter->second;
//	}
//
//	_LOG(USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"GetUser - Failed.. SessionId : %llu", sessionId);
//	return UserPtr();
//}