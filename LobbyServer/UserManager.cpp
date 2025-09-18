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
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[CreateUser] - Sessionid %llu�� �ߺ��˴ϴ�.", sessionId);
		return nullptr;
	}

	// ���� �α��� �۾��� ���� ������ �̷��� UserId�� ����.
	// �̰� �ƴ϶�� �Ƹ� ID�� ���;߰���?
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
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[RemoveUser] - sessionId : [%llu]�� �ش��ϴ� ������ �������� �ʽ��ϴ�.", sessionId);
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
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[GetUserByUserId] - userId : %llu�� �ش��ϴ� ������ �������� �ʽ��ϴ�.", userId);
		return UserPtr(nullptr);
	}
	
	return m_userIdToUserUMap[userId];
}

UserPtr jh_content::UserManager::GetUserBySessionId(ULONGLONG sessionId)
{
	auto userIter = m_sessionIdToUserUMap.find(sessionId);

	if (userIter == m_sessionIdToUserUMap.end())
	{
		_LOG(L"UserManager", LOG_LEVEL_WARNING, L"[GetUserByUserId] - sessionId : [%llu]�� �ش��ϴ� ������ �������� �ʽ��ϴ�.", sessionId);
		return UserPtr(nullptr);
	}

	return m_sessionIdToUserUMap[sessionId];
}