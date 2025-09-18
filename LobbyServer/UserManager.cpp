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
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[CreateUser] - Sessionid %llu가 중복됩니다.", sessionId);
		return nullptr;
	}

	// 현재 로그인 작업이 없기 때문에 이렇게 UserId를 얻어옴.
	// 이게 아니라면 아마 ID를 얻어와야겠지?
	static ULONGLONG newUserId = 4283;

	const static ULONGLONG mod = (ULONGLONG)1 << 63;

	newUserId = (newUserId + 3) % mod;

	UserPtr userPtr = std::make_shared<jh_content::User>(sessionId, newUserId);

	m_sessionIdToUserUMap[sessionId] = userPtr;
	m_userIdToUserUMap[newUserId] = userPtr;
	
	std::cout << "[CreateUser] UserId : " << newUserId << " SessionID : " << sessionId << "\n";
	return userPtr;
}

void jh_content::UserManager::RemoveUser(ULONGLONG sessionId)
{
	auto findIt = m_sessionIdToUserUMap.find(sessionId);

	if (findIt == m_sessionIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[RemoveUser] - sessionId : [%llu]에 해당하는 유저가 존재하지 않습니다.", sessionId);
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
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[GetUserByUserId] - userId : %llu에 해당하는 유저가 존재하지 않습니다.", userId);
		return UserPtr(nullptr);
	}
	
	return m_userIdToUserUMap[userId];
}

UserPtr jh_content::UserManager::GetUserBySessionId(ULONGLONG sessionId)
{
	auto userIter = m_sessionIdToUserUMap.find(sessionId);

	if (userIter == m_sessionIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[GetUserByUserId] - sessionId : [%llu]에 해당하는 유저가 존재하지 않습니다.", sessionId);
		return UserPtr(nullptr);
	}

	return m_sessionIdToUserUMap[sessionId];
}