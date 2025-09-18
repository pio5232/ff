#include "pch.h"
#include "GamePlayer.h"
#include "UserManager.h"
#include "GameServer.h"
#include "PacketHandler.h"
#include "GameSession.h"
#include "AIPlayer.h"
#include "GameWorld.h"
#include "Memory.h"
using namespace jh_network;
//GamePlayerPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId, ULONGLONG userId)
UserPtr jh_content::UserManager::CreateUser(ULONGLONG sessionId, ULONGLONG userId)
{
	auto sessionIt = m_sessionIdToUserUMap.find(sessionId);
	auto userIt = m_userIdToUserUMap.find(userId);

	if (sessionIt == m_sessionIdToUserUMap.end())
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[CreateUser] - 중복된 세션입니다. SessionId : [%llu]", sessionId);
		return;
	}

	if (userIt == m_userIdToUserUMap.end())
	{
		_LOG(GAME_USER_MANAGER_SAVE_FILE_NAME, LOG_LEVEL_SYSTEM, L"[CreateUser] - 중복된 유저ID입니다. UserId : [%llu]", userId);
		return;
	}

	UserPtr userPtr = MakeShared<jh_content::User>(g_memAllocator, sessionId, userId);
	

	//GamePlayerPtr gamePlayerPtr = std::make_shared<GamePlayer>(gameSessionPtr, worldPtr);

	//m_userIdToPlayerDic.insert({ gameSessionPtr->GetUserId(), gamePlayerPtr });

	//printf("CreatePlayer - GameSession User ID = [ %llu ]\n ", gameSessionPtr->GetUserId());
	//return gamePlayerPtr;
}



ErrorCode jh_content::UserManager::RemoveUser(ULONGLONG userId)
{
	{
		if (m_userIdToPlayerDic.find(userId) == m_userIdToPlayerDic.end())
			return ErrorCode::NOT_FOUND;

		m_userIdToPlayerDic.erase(userId);
	}

	return ErrorCode::NONE;
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
//	_aliveGamePlayerCount.fetch_sub(1);
//
//	return ErrorCode::NONE;
//}

UserPtr jh_content::UserManager::GetUserByUserId(ULONGLONG userId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_userIdToUserUMap.find(userId);

	if (iter != m_userIdToUserUMap.end())
		return iter->second;

	return nullptr;
}

UserPtr jh_content::UserManager::GetUserBySessionId(ULONGLONG sessionId)
{
	std::unordered_map<ULONGLONG, UserPtr>::iterator iter = m_sessionIdToUserUMap.find(sessionId);

	if (iter != m_sessionIdToUserUMap.end())
		return iter->second;

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

