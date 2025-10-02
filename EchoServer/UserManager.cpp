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
//	// ���� �α��� �۾��� ���� ������ �̷��� UserId�� ����.
//	// �̰� �ƴ϶�� �Ƹ� ID�� ���;߰���?
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
//	// �ش� ���� User�� �����ϴ� ���
//	if (findIter == userInfo.end())
//	{
//		return findIter->second;
//	}
//
//	_LOG(USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_INFO, L"GetUser - Failed.. SessionId : %llu", sessionId);
//	return UserPtr();
//}