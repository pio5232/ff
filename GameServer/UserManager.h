#pragma once
#include <unordered_map>

namespace jh_content
{
	class GameWorld;
	class UserManager //: public jh_utility::JobQueue
	{
	public:

		UserManager(SendPacketFunc sendPacketFunc) : m_sendPacketFunc(sendPacketFunc) {}
		UserManager(const UserManager&) = delete;

		~UserManager() 
		{
			m_sessionIdToUserUMap.clear();
			m_userIdToUserUMap.clear();
			m_entityIdToUserUMap.clear();
		}

		UserManager& operator=(const UserManager&) = delete;

		UserPtr CreateUser(ULONGLONG sessionId, ULONGLONG userId);
		void RemoveUser(ULONGLONG userId);

		void Unicast(ULONGLONG sessionId, PacketRef& packet);
		void Broadcast(PacketRef& packet);

		USHORT GetPlayerCount() const { return static_cast<USHORT>(m_userIdToUserUMap.size()); }

		void ReserveUMapSize(USHORT requiredUsers, USHORT maxUsers);

		// 모든 플레이어에 대한 MakeOtherPacket 생성
		const std::unordered_map<ULONGLONG, UserPtr>& GetAllPlayersByUserId() const
		{
			return m_userIdToUserUMap;
		}

		USHORT GetUserCount() { return m_sessionIdToUserUMap.size(); }
		
		void RegisterEntityIdToUser(ULONGLONG entityId, UserPtr userPtr);
		void DeleteEntityIdToUser(ULONGLONG entityId);
		
		UserPtr GetUserByUserId(ULONGLONG userId);
		UserPtr GetUserBySessionId(ULONGLONG sessionId);
		UserPtr GetUserByEntityId(ULONGLONG entityId);
	private:
		SendPacketFunc m_sendPacketFunc;

		std::unordered_map<ULONGLONG, UserPtr> m_sessionIdToUserUMap;
		std::unordered_map<ULONGLONG, UserPtr> m_userIdToUserUMap;

		// EntityID
		std::unordered_map<ULONGLONG, UserPtr> m_entityIdToUserUMap;


	};
}