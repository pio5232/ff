#pragma once

namespace jh_content
{
	class WorldChat : public jh_utility::JobQueue
	{
	public:
		ErrorCode Chat(jh_network::SharedSendBuffer sharedSendBuffer);

		void RegisterMember(GameSessionPtr gameSessionPtr);
		void RemoveMember(ULONGLONG userId);

	private:

		// userId, session
		std::unordered_map<ULONGLONG, std::weak_ptr<class jh_network::GameSession>> _userIdToSessionMap;

	};
}

