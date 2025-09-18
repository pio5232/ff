#include "pch.h"
#include "WorldChat.h"
#include "GameSession.h"

ErrorCode jh_content::WorldChat::Chat(jh_network::SharedSendBuffer sharedSendBuffer)
{
	for (auto& [id, weakGameSessionPtr] : _userIdToSessionMap)
	{
		if (weakGameSessionPtr.expired())
			continue;

		GameSessionPtr gameSessionPtr = weakGameSessionPtr.lock();

		gameSessionPtr->Send(sharedSendBuffer);
	}

	return ErrorCode::NONE;
}

void jh_content::WorldChat::RegisterMember(GameSessionPtr gameSessionPtr)
{
	ULONGLONG userId = gameSessionPtr->GetUserId();

	_userIdToSessionMap.insert(std::make_pair(userId, gameSessionPtr));
}

void jh_content::WorldChat::RemoveMember(ULONGLONG userId)
{
	if (_userIdToSessionMap.contains(userId))
		_userIdToSessionMap.erase(userId);
}
