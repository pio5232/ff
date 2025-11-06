#include "pch.h"
#include "GamePlayer.h"
#include "UserManager.h"
#include "GameServer.h"
#include "AIPlayer.h"
#include "GameWorld.h"
#include "Memory.h"
#include "User.h"
using namespace jh_network;

UserPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId, ULONGLONG userId)
{
	auto sessionIt = m_sessionIdToUserUMap.find(sessionId);
	auto userIt = m_userIdToUserUMap.find(userId);

	if (sessionIt != m_sessionIdToUserUMap.end())
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[CreateUser] Duplicate session. SessionId : [0x%016llx]", sessionId);
		return nullptr;
	}

	if (userIt != m_userIdToUserUMap.end())
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[CreateUser] Duplicate user. UserId : [%llu]", userId);
		return nullptr;
	}

	UserPtr userPtr = MakeShared<jh_content::User>(g_memSystem, sessionId, userId);
	
	m_sessionIdToUserUMap.insert({ sessionId, userPtr });
	m_userIdToUserUMap.insert({ userId, userPtr });
	
	return userPtr;

}



void jh_content::UserManager::RemoveUser(ULONGLONG sessionId)
{
	UserPtr userPtr = GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[RemoveUser] User not found. SessionId: [0x%016llx]", sessionId);
		return;
	}

	ULONGLONG userId = userPtr->GetUserId();

	m_sessionIdToUserUMap.erase(sessionId);
	m_userIdToUserUMap.erase(userId);
}

void jh_content::UserManager::Unicast(ULONGLONG sessionId, PacketPtr& packet)
{
	m_sendPacketFunc(sessionId, packet);
}

void jh_content::UserManager::Broadcast(PacketPtr& packet)
{
	for (const auto& [sessionId, userPtr] : m_sessionIdToUserUMap)
	{
		m_sendPacketFunc(sessionId, packet);
	}
}


void jh_content::UserManager::RegisterEntityIdToUser(ULONGLONG entityId, UserPtr userPtr)
{
	if (m_entityIdToUserUMap.end() != m_entityIdToUserUMap.find(entityId))
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[RegisterEntityIdToUser] Duplicated entity registration. EntityId : [%llu]", entityId);

		return;
	}

	m_entityIdToUserUMap.insert({ entityId, userPtr });

}

void jh_content::UserManager::DeleteEntityIdToUser(ULONGLONG entityId)
{
	m_entityIdToUserUMap.erase(entityId);
}

UserPtr jh_content::UserManager::GetUserByUserId(ULONGLONG userId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_userIdToUserUMap.find(userId);

	if (iter != m_userIdToUserUMap.end())
		return iter->second;

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetUserByUserId] User not found. UserId : [%llu]", userId);

	return nullptr;
}

UserPtr jh_content::UserManager::GetUserBySessionId(ULONGLONG sessionId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_sessionIdToUserUMap.find(sessionId);

	if (iter != m_sessionIdToUserUMap.end())
		return iter->second;

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetUserBySessionId] User not found. SessionId : [0x%016llx]", sessionId);

	return nullptr;
}

UserPtr jh_content::UserManager::GetUserByEntityId(ULONGLONG entityId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_entityIdToUserUMap.find(entityId);

	if (iter != m_entityIdToUserUMap.end())
		return iter->second;

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetUserByEntityId] User not found. EntityId : [%llu]", entityId);
	return nullptr;
}

void jh_content::UserManager::ReserveUMapSize(USHORT requiredUsers, USHORT maxUsers)
{
	// 이거 서버의 maxCnt가 0이라서 받지 않음.
	m_sessionIdToUserUMap.reserve(requiredUsers);
	m_userIdToUserUMap.reserve(requiredUsers);
}