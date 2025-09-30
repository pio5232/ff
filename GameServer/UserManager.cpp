#include "pch.h"
#include "GamePlayer.h"
#include "UserManager.h"
#include "GameServer.h"
#include "AIPlayer.h"
#include "GameWorld.h"
#include "Memory.h"
#include "User.h"
using namespace jh_network;
//GamePlayerPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId, ULONGLONG userId)
UserPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId, ULONGLONG userId)
{
	auto sessionIt = m_sessionIdToUserUMap.find(sessionId);
	auto userIt = m_userIdToUserUMap.find(userId);

	if (sessionIt != m_sessionIdToUserUMap.end())
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[CreateUser] - 중복된 세션입니다. SessionId : [%llu]", sessionId);
		return nullptr;
	}

	if (userIt != m_userIdToUserUMap.end())
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[CreateUser] - 중복된 유저ID입니다. UserId : [%llu]", userId);
		return nullptr;
	}

	UserPtr userPtr = MakeShared<jh_content::User>(g_memAllocator, sessionId, userId);
	
	m_sessionIdToUserUMap.insert({ sessionId, userPtr });
	m_userIdToUserUMap.insert({ userId, userPtr });
	
	return userPtr;
	//GamePlayerPtr gamePlayerPtr = std::make_shared<GamePlayer>(gameSessionPtr, worldPtr);

	//m_userIdToPlayerDic.insert({ gameSessionPtr->GetUserId(), gamePlayerPtr });

	//printf("CreatePlayer - GameSession User ID = [ %llu ]\n ", gameSessionPtr->GetUserId());
	//return gamePlayerPtr;
}



void jh_content::UserManager::RemoveUser(ULONGLONG sessionId)
{
	UserPtr userPtr = GetUserBySessionId(sessionId);

	if (nullptr == userPtr)
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[RemoveUser] - 유저가 존재하지 않습니다. SessionId : [%llu]", sessionId);
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

//ErrorCode jh_content::UserManager::DeleteAI(ULONGLONG userId)
//{
//	SRWLockGuard lockGuard(&_aiLock);
//
//	if (_idToAiDic.find(userId) == _idToAiDic.end())
//		return ErrorCode::NOT_FOUND;
//
//	_idToAiDic.erase(userId);
//
//	aliveGamePlayerCount.fetch_sub(1);
//
//	return ErrorCode::NONE;
//}


void jh_content::UserManager::RegisterEntityIdToUser(ULONGLONG entityId, UserPtr userPtr)
{
	if (m_entityIdToUserUMap.end() != m_entityIdToUserUMap.find(entityId))
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[RegistPlayer] - 중복된 플레이어 등록입니다. EntityId : [%llu]", entityId);

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

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetUserByUserId] - 유저 검색 성공. UserId : [%llu]", userId);

	return nullptr;
}

UserPtr jh_content::UserManager::GetUserBySessionId(ULONGLONG sessionId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_sessionIdToUserUMap.find(sessionId);

	if (iter != m_sessionIdToUserUMap.end())
		return iter->second;

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetUserBySessionId] - 세션 검색 성공. SessionId : [%llu]", sessionId);

	return nullptr;
}

UserPtr jh_content::UserManager::GetUserByEntityId(ULONGLONG entityId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_entityIdToUserUMap.find(entityId);

	if (iter != m_entityIdToUserUMap.end())
		return iter->second;

	_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_WARNING, L"[GetUserByEntityId] - 개체 검색 성공. EntityId : [%llu]", entityId);

	return nullptr;
}

void jh_content::UserManager::ReserveUMapSize(USHORT requiredUsers, USHORT maxUsers)
{
	// 이거 서버의 maxCnt가 0이라서 받지 않음.
	m_sessionIdToUserUMap.reserve(requiredUsers);
	m_userIdToUserUMap.reserve(requiredUsers);
}

//void jh_content::UserManager::MakeUserCharactersPacket(jh_network::SharedSendBuffer& sendBuffer)
//{
//	std::vector<GamePlayerPtr> gamePlayersVec;
//	{
//		SRWLockGuard lockGuard(&_playerLock);
//
//		for (auto& [userId, gamePlayerPtr] : m_userIdToPlayerDic)
//		{
//			gamePlayersVec.push_back(gamePlayerPtr);
//		}
//	}
//
//	jh_network::MakeOtherCharacterPacket makeOtherPacket;
//	for (GamePlayerPtr& gamePlayer : gamePlayersVec)
//	{
//		makeOtherPacket.entityId = gamePlayer->GetEntityId();
//		makeOtherPacket.pos = gamePlayer->GetPosition();
//
//		*sendBuffer << makeOtherPacket.size << makeOtherPacket.type << makeOtherPacket.entityId << makeOtherPacket.pos;
//	}
//
//	return;
//}

