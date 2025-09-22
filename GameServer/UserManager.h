#pragma once
#include <unordered_map>

namespace jh_content
{
	class GameWorld;
	class UserManager //: public jh_utility::JobQueue
	{
	public:
		using SendPacketFunc = std::function<void(ULONGLONG, PacketPtr&)>; // sessionId

		UserManager(SendPacketFunc sendPacketFunc) : m_sendPacketFunc(sendPacketFunc) {}
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

		// ��� �÷��̾ ���� MakeOtherPacket ����
		const std::unordered_map<ULONGLONG, UserPtr>& GetAllPlayersByUserId() const
		{
			return m_userIdToUserUMap;
		}

		~UserManager() 
		{
			m_sessionIdToUserUMap.clear();
			m_userIdToUserUMap.clear();
			m_entityIdToUserUMap.clear();
		}
		
		void RegisterEntityIdToUser(ULONGLONG entityId, UserPtr userPtr);
		void DeleteEntityIdToUser(ULONGLONG entityId);
		
		UserPtr GetUserByUserId(ULONGLONG userId);
		UserPtr GetUserBySessionId(ULONGLONG sessionId);
		UserPtr GetUserByEntityId(ULONGLONG entityId);
	private:
		SendPacketFunc m_sendPacketFunc;
		//std::unordered_map<ULONGLONG, GamePlayerPtr> m_sessionIdToPlayerDic;
		//std::unordered_map<ULONGLONG, GamePlayerPtr> m_userIdToPlayerDic;

		std::unordered_map<ULONGLONG, UserPtr> m_sessionIdToUserUMap;
		std::unordered_map<ULONGLONG, UserPtr> m_userIdToUserUMap;

		// EntityID
		std::unordered_map<ULONGLONG, UserPtr> m_entityIdToUserUMap;


	};
}