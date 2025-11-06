#include "pch.h"
#include "UserManager.h"
#include "User.h"

/// +---------------------------+
/// |		UserManager			|
/// +---------------------------+

jh_content::UserManager::UserManager()
{
}

jh_content::UserManager::~UserManager()
{
}

void jh_content::UserManager::Init()
{
}

UserPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId)
{
	if (m_sessionIdToUserUMap.find(sessionId) != m_sessionIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[CreateUser] Duplicate SessionId: [0x%016llx]", sessionId);
		return nullptr;
	}

	static ULONGLONG newUserId = 4283;
	newUserId += 3;

	UserPtr userPtr = std::make_shared<jh_content::User>(sessionId, newUserId);

	m_sessionIdToUserUMap[sessionId] = userPtr;
	m_userIdToUserUMap[newUserId] = userPtr;
	
	return userPtr;
}

void jh_content::UserManager::RemoveUser(ULONGLONG sessionId)
{
	auto findIt = m_sessionIdToUserUMap.find(sessionId);

	if (findIt == m_sessionIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[RemoveUser] User not found. SessionId: [0x%016llx]", sessionId);
		return;
	}

	ULONGLONG userId = findIt->second->GetUserId();

	m_userIdToUserUMap.erase(userId);
	m_sessionIdToUserUMap.erase(sessionId);
}

UserPtr jh_content::UserManager::GetUserByUserId(ULONGLONG userId)
{
	auto userIter = m_userIdToUserUMap.find(userId);

	if (userIter == m_userIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[GetUserByUserId] User not found. UserId: [%llu]", userId);
		return UserPtr(nullptr);
	}
	
	return m_userIdToUserUMap[userId];
}

UserPtr jh_content::UserManager::GetUserBySessionId(ULONGLONG sessionId)
{
	auto userIter = m_sessionIdToUserUMap.find(sessionId);

	// 세션만 연결된 경우에는 유저가 존재하지 않음.
	if (userIter == m_sessionIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_INFO, L"[GetUserBySessionId] User not found. SessionId: [0x%016llx]", sessionId);	
		return UserPtr(nullptr);
	}

	return m_sessionIdToUserUMap[sessionId];
}