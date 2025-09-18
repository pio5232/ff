#pragma once
#include <unordered_map>

namespace jh_content
{
	class GameWorld;
	class UserManager //: public jh_utility::JobQueue
	{
	public:
		using SendPacketFunc = std::function<void(ULONGLONG, PacketPtr&)>; // sessionId

		UserManager() : m_sendPacketFunc{} {}
		UserManager(const UserManager&) = delete;
		UserManager& operator=(const UserManager&) = delete;

		UserPtr CreateUser(ULONGLONG sessionId, ULONGLONG userId);
		ErrorCode RemoveUser(ULONGLONG userId);
		//ErrorCode DeleteAI(ULONGLONG userId);

		//ErrorCode SendToPlayer(PacketPtr packet, ULONGLONG userId);
		//ErrorCode SendToAllPlayer(PacketPtr packet);
		void Unicast(ULONGLONG sessionId, PacketPtr& packet);
		void Broadcast(PacketPtr& packet);

		USHORT GetPlayerCount() const { return static_cast<USHORT>(m_userIdToUserUMap.size()); }

		void ReserveUMapSize(USHORT requiredUsers, USHORT maxUsers);

		// 모든 플레이어에 대한 MakeOtherPacket 생성
		const std::unordered_map<ULONGLONG, UserPtr>& GetAllPlayersByUserId() const
		{
			return m_userIdToUserUMap;
		}

		~UserManager() 
		{
			m_sessionIdToUserUMap.clear();
			m_userIdToUserUMap.clear();
		}
		
		UserPtr GetUserByUserId(ULONGLONG userId);
		UserPtr GetUserBySessionId(ULONGLONG sessionId);

	private:
		SendPacketFunc m_sendPacketFunc;
		//std::unordered_map<ULONGLONG, GamePlayerPtr> m_sessionIdToPlayerDic;
		//std::unordered_map<ULONGLONG, GamePlayerPtr> m_userIdToPlayerDic;

		std::unordered_map<ULONGLONG, UserPtr> m_sessionIdToUserUMap;
		std::unordered_map<ULONGLONG, UserPtr> m_userIdToUserUMap;
	};
}