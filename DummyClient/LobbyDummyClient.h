#pragma once

namespace jh_content
{
	class LobbyDummyClient : public jh_network::IocpClient
	{
	public:
		LobbyDummyClient();
		virtual ~LobbyDummyClient();

		// IocpClient을(를) 통해 상속됨
		void OnRecv(ULONGLONG sessionId, PacketPtr dataBuffer, USHORT type) override;

		void OnConnected(ULONGLONG sessionId) override;


		void HandleRoomListResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleLogInResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleMakeRoomResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleEnterRoomResponsePacket(ULONGLONG session, PacketPtr& packet);
		void HandleChatNotifyPacket(ULONGLONG session, PacketPtr& packet);
		void HandleLeaveRoomResponsePacket(ULONGLONG session, PacketPtr& packet);



	};
}

