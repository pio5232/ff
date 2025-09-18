#pragma once
 
using serializationBuffer = jh_utility::SerializationBuffer;

using namespace jh_content;

namespace jh_network
{
	const int ServerPort = 6000;
	const int LanServerPort = 8768;

	const int MAX_PACKET_SIZE = 1000;
	const ULONGLONG HEARTBEAT_TIMEOUT = 30000; // MS, 30��

	// AAA �����̳� ������ ������ ��Ŷ���� ��ġ����?. (EX) EnterRoom / LeaveRoom Request Packet�� ���� ��Ұ� �Ȱ��� ������ �ϳ��� ��ġ�� �÷��׸� �����ϴ� ������ ����, (EnterRoom Notify / LeaveRoomNotify)

	/// <summary>
	/// ���Ǽ��� ���ؼ� ����ü�� �����ϰ� ������, ������ �Ǹ� ����ü�� �����ص���..
	/// </summary>
	enum PacketType : USHORT // packet order
	{
		INVALID_PACKET = 65535,
		// C_S, REQUEST = Ŭ���̾�Ʈ -> ����
		// S_C, RESPONSE = ���� -> Ŭ���̾�Ʈ
		LOG_IN_REQUEST_PACKET = 0, //
		LOG_IN_RESPONSE_PACKET,

		ROOM_LIST_REQUEST_PACKET,
		ROOM_LIST_RESPONSE_PACKET,

		MAKE_ROOM_REQUEST_PACKET,
		MAKE_ROOM_RESPONSE_PACKET, // -> ENTER_ROOM_RESPONSE_PACKET���� RESPONSE�� ����.

		ENTER_ROOM_REQUEST_PACKET,
		ENTER_ROOM_RESPONSE_PACKET,
		ENTER_ROOM_NOTIFY_PACKET,

		LEAVE_ROOM_REQUEST_PACKET,
		LEAVE_ROOM_RESPONSE_PACKET,
		LEAVE_ROOM_NOTIFY_PACKET,

		CLIENT_LOG_OUT_PACKET,
		CLIENT_LOG_OUT_NOTIFY_PACKET,

		OWNER_CHANGE_NOTIFY_PACKET,
		// Ŭ���̾�Ʈ�� ��û
		CHAT_TO_ROOM_REQUEST_PACKET, // �� ���� �������� �޽��� ���� ��û, RoomNum�� -1�̸� ��� �濡 ����.
		CHAT_TO_USER_REQUEST_PACKET, // �ϳ��� �������� �޽��� ���� ��û.
		CHAT_NOTIFY_PACKET,

		// ������ ����.
		CHAT_TO_ROOM_RESPONSE_PACKET,
		CHAT_TO_USER_RESPONSE_PACKET, // 

		GAME_READY_REQUEST_PACKET,
		//GAME_READY_RESPONSE_PACKET,
		GAME_READY_NOTIFY_PACKET,
		GAME_START_NOTIFY_PACKET,

		ERROR_PACKET, // Ŭ���̾�Ʈ���� ������ �������� ��ȿ���� ���� ��û�� �Ǿ �������� ������ �Բ� �����ϵ��� �Ѵ�. ex) ������ �濡 ������ �ϴ� ����

		// -- LAN
		GAME_SERVER_SETTING_REQUEST_PACKET, // 1. ó�� GAME LOBBY�� �����ϸ� ������ ��û��Ŷ.(GAME -> LOBBY)
		GAME_SERVER_SETTING_RESPONSE_PACKET, // 2. ���� ������ ������ �� �������� ���� (LOBBY -> GAME)
		GAME_SERVER_LAN_INFO_PACKET, // 3. �ڽ� ip,port �� ���� ���� (GAME -> LOBBY) �̰� ������ �濡 �ִ� ���̵鿡�� �ٷ� ����.

		// -- GAME
		GAME_INIT_DONE_PACKET,
		ENTER_GAME_REQUEST_PACKET,
		ENTER_GAME_RESPONSE_PACKET,
		LEAVE_GAME_NOTIFY_PACKET,

		MAKE_MY_CHARACTER_PACKET,
		MAKE_OTHER_CHARACTER_PACKET,
		DEL_OTHER_CHARACTER_PACKET,

		GAME_LOAD_COMPELTE_PACKET, // MAKE PLAYER �� �� �۾��� �Ǿ��ٸ�.

		MOVE_START_REQUEST_PACKET,
		MOVE_START_NOTIFY_PACKET, // RESPONSE �� NOTIFY. ��� �������� ����
		MOVE_STOP_REQUEST_PACKET,
		MOVE_STOP_NOTIFY_PACKET, // RESPONSE �� NOTIFY. ��� �������� ����.

		ATTACK_REQUEST_PACKET,
		ATTACK_NOTIFY_PACKET,

		ATTACKED_NOTIFY_PACKET,
		DIE_NOTIFY_PACKET,
		SEPCTATOR_INIT_PACKET,

		// ���� ���� ��Ŷ
		GAME_END_NOTIFY_PACKET,
		
		// (��� ������)�� �˸�
		UPDATE_WINNER_NOTIFY_PACKET,
		
		// ��� �����ڰ� ������ ������� �˸�
		INVALIDATE_WINNER_NOTIFY_PACKET,
		
		// ���� ����� ������ ���� (���� �ÿ�)
		WINNER_INFO_NOTIFY_PACKET,

		CHARACTER_POSITION_SYNC_PACKET,
		UPDATE_TRANSFORM_PACKET,

		HEART_BEAT_PACKET, // ���� ������ ���� ��Ŷ, 5�ʿ� �ϳ��� �������� �Ѵ�. // 30�ʰ� ������ ������ ���� �ɷ� üũ.

		ECHO_PACKET = 65534,
	};

	enum PacketErrorCode : USHORT
	{
		REQUEST_DESTROYED_ROOM = 0, // �̹� ������ �濡 ���� ��û
		REQUEST_DIFF_ROOM_NAME, // ��û�� �� ������ �ٸ���.

		FULL_ROOM, // �ο��� �� á��.
		ALREADY_RUNNING_ROOM, // �̹� ������ ���۵� ���̴�.

		// -- GAME
		CONNECTED_FAILED_WRONG_TOKEN, // ���� ������ ������ �� ��ū�� �߸��� ��ū�̴�.
	};
	// �׻� padding�� �����ϴ��� Ȯ���ؾ��Ѵ�.
	enum PacketSize
	{
		PACKET_SIZE_ECHO = 10,

		PACKET_SIZE_MAX = 500,
	};

	// size = payload�� size
	// ��Ʈ��ũ �ڵ忡�� PacketHeader�� �и��ϰ� �Ǹ� PacketHeader��ŭ�� ������� ������� �ʴ´�.
	struct PacketHeader
	{
	public:
		//void SetSize(WORD headerSize) { size = headerSize; }
		USHORT size = 0;
		USHORT type = INVALID_PACKET;
	};

	// ECHO REQUEST / RESPONSE
	//struct EchoPacket : public PacketHeader
	//{
	//public:
	//	EchoPacket() : m_data(0) { size = sizeof(__int64), type = ECHO_PACKET; }
	//	__int64 m_data;
	//};

	struct ErrorPacket : public PacketHeader
	{
		ErrorPacket() { size = sizeof(packetErrorCode); type = ERROR_PACKET; }
		USHORT packetErrorCode = 0;
	};
	// CHAT 

	struct ChatUserRequestPacket : public PacketHeader
	{
	public:
		//ULONGLONG sendUserId = 0;
		ULONGLONG targetUserId = 0;
		USHORT messageLen = 0;
		WCHAR payLoad[0];		//WCHAR payLoad[MESSAGE_MAX_LEN];

	};
	struct ChatRoomRequestPacket : public PacketHeader
	{
		// AA
	public:
		USHORT roomNum = UINT16_MAX;
		USHORT messageLen = 0;
		WCHAR payLoad[0];
	};
	
	// head�� ����.
	struct ChatUserResponsePacket : public PacketHeader
	{
		ChatUserResponsePacket(){ type = CHAT_TO_USER_RESPONSE_PACKET; }
	};
	
	struct ChatRoomResponsePacket : public PacketHeader 
	{
		ChatRoomResponsePacket() { type = CHAT_TO_ROOM_RESPONSE_PACKET; }
	};

	struct LeaveRoomResponsePacket : public PacketHeader
	{
		LeaveRoomResponsePacket() { type = LEAVE_ROOM_RESPONSE_PACKET; }
	};





	struct ChatOtherUserNotifyPacket : public PacketHeader
	{
	public :
		ChatOtherUserNotifyPacket() { type = CHAT_NOTIFY_PACKET; }
		ULONGLONG sendUserId = 0;
		USHORT messageLen = 0;
		WCHAR payLoad[0];
	};
	// LOG_IN
	// ��ȣȭ.. ��ȣȭ?
	struct alignas (32) LogInRequestPacket : public PacketHeader
	{
	public:
		LogInRequestPacket() { size = sizeof(logInId) + sizeof(logInPw); type = LOG_IN_REQUEST_PACKET; }
		ULONGLONG logInId = 0;
		ULONGLONG logInPw = 0;
	};

	struct alignas (16) LogInResponsePacket : public PacketHeader
	{
		LogInResponsePacket() { size = sizeof(userId); type = LOG_IN_RESPONSE_PACKET; }
		ULONGLONG userId = 0;
	};

	// LOG_OUT
	struct LogOutRequestPacket : public PacketHeader
	{

	};
	struct LogOutResponsePacket : public PacketHeader
	{

	};

	// OWNER CHANGE NOTIFY
	struct OwnerChangeNotifyPacket : PacketHeader
	{
		OwnerChangeNotifyPacket() { size = sizeof(userId); type = OWNER_CHANGE_NOTIFY_PACKET; }
		ULONGLONG userId = 0;
	};

	struct LogOutNotifyPacket : PacketHeader
	{
		LogOutNotifyPacket() { size = sizeof(userId); type = CLIENT_LOG_OUT_NOTIFY_PACKET; }
		ULONGLONG userId = 0;
	};

	struct MakeRoomRequestPacket : public PacketHeader
	{
		MakeRoomRequestPacket() { size = sizeof(roomName); type = MAKE_ROOM_REQUEST_PACKET; }
		WCHAR roomName[ROOM_NAME_MAX_LEN]{};
	};

	struct MakeRoomResponsePacket : public PacketHeader
	{
		MakeRoomResponsePacket() { size = sizeof(isMade) + RoomInfo::GetSize();  type = MAKE_ROOM_RESPONSE_PACKET; }
		bool isMade = false;
		RoomInfo roomInfo{};
	};

	//struct alignas (64) EnterRoomResponsePacket : public PacketHeader
	struct EnterRoomResponsePacket : public PacketHeader
	{
		EnterRoomResponsePacket() {  type = ENTER_ROOM_RESPONSE_PACKET; } 
		bool bAllow = false;
		USHORT idCnt = 0;
		ULONGLONG ids[0]; // ������ id �������� �ֻ��� bit�� Ready ���¸� �������� �Ѵ�.

		//RoomInfo roomInfo = {};
	};

	// ENTER_ROOM
	struct EnterRoomRequestPacket : public PacketHeader
	{
		EnterRoomRequestPacket() { size = sizeof(roomNum) + sizeof(roomName); type = ENTER_ROOM_REQUEST_PACKET; }
		USHORT roomNum = 0;
		WCHAR roomName[ROOM_NAME_MAX_LEN]{};
	};

	// other notify
	struct EnterRoomNotifyPacket : public PacketHeader
	{
		EnterRoomNotifyPacket() { size = sizeof(enterUserId); type = ENTER_ROOM_NOTIFY_PACKET; }
		ULONGLONG enterUserId = 0;
	};

	struct LeaveRoomNotifyPacket : public PacketHeader
	{
		LeaveRoomNotifyPacket() { size = sizeof(leaveUserId); type = LEAVE_ROOM_NOTIFY_PACKET; }
		ULONGLONG leaveUserId = 0;
	};

	// LEAVE_ROOM
	struct LeaveRoomRequestPacket : public PacketHeader
	{
		LeaveRoomRequestPacket() { size = sizeof(roomNum) + sizeof(roomName); type = LEAVE_ROOM_REQUEST_PACKET; }
		USHORT roomNum = 0;
		WCHAR roomName[ROOM_NAME_MAX_LEN]{};
	};

	// REQUEST ROOM LIST
	struct RoomListRequestPacket : public PacketHeader
	{
	public:
		RoomListRequestPacket() { type = ROOM_LIST_REQUEST_PACKET; }
	};
	struct RoomListResponsePacket : public PacketHeader
	{
		RoomListResponsePacket() { type = ROOM_LIST_RESPONSE_PACKET; }
		USHORT roomCnt = 0;
		RoomInfo roomInfos[0];
	};	

	// GameReadyPacket
	struct GameReadyRequestPacket : public PacketHeader
	{
	public:
		GameReadyRequestPacket() { type = GAME_READY_REQUEST_PACKET; size = sizeof(isReady); }
		bool isReady = false;
	};

	struct GameReadyNotifyPacket : public PacketHeader
	{
	public:
		GameReadyNotifyPacket() { type = GAME_READY_NOTIFY_PACKET; size = sizeof(isReady) + sizeof(userId); }
		bool isReady = false;
		ULONGLONG userId = 0;
	};


	struct GameStartNotifyPacket : public PacketHeader
	{
		GameStartNotifyPacket() { type = GAME_START_NOTIFY_PACKET;}
	};
}

// ---- LAN
namespace jh_network
{
	struct GameServerSettingRequestPacket :public PacketHeader
	{
		GameServerSettingRequestPacket() { type = GAME_SERVER_SETTING_REQUEST_PACKET; }
	};

	struct GameServerSettingResponsePacket :public PacketHeader
	{
		GameServerSettingResponsePacket() { type = GAME_SERVER_SETTING_RESPONSE_PACKET; size = sizeof(roomNum) + sizeof(requiredUsers) + sizeof(maxUsers); }

		USHORT roomNum = 0;
		USHORT requiredUsers = 0; // �ε� ���� ��
		USHORT maxUsers = 0; // Max, Max - required�� AI�� ����� ��.
	};

	struct GameServerLanInfoPacket :public PacketHeader
	{
		GameServerLanInfoPacket() { type = GAME_SERVER_LAN_INFO_PACKET; size = sizeof(ipStr) + sizeof(port) + sizeof(roomNum) + sizeof(xorToken); }

		WCHAR ipStr[IP_STRING_LEN] = {};
		USHORT port = 0;
		USHORT roomNum = 0;
		ULONGLONG xorToken = 0;
	};


}

// ------ GAME
namespace jh_network
{
	struct GameInitDonePacket : public PacketHeader
	{
		GameInitDonePacket() { type = GAME_INIT_DONE_PACKET; }
	};

	struct EnterGameRequestPacket :public PacketHeader
	{
		EnterGameRequestPacket() { type = ENTER_GAME_REQUEST_PACKET; size = sizeof(userId) + sizeof(token); }

		ULONGLONG userId = 0;
		ULONGLONG token = 0;
	};

	struct EnterGameResponsePacket : public PacketHeader
	{
		EnterGameResponsePacket() { type = ENTER_GAME_RESPONSE_PACKET; }
	};

	struct LeaveGameNotifyPacket : public PacketHeader
	{
		LeaveGameNotifyPacket() { type = LEAVE_GAME_NOTIFY_PACKET; size = sizeof(entityId); }
		ULONGLONG entityId = 0;
	};

	struct MakeMyCharacterPacket : public PacketHeader
	{
		MakeMyCharacterPacket() { type = MAKE_MY_CHARACTER_PACKET; size = sizeof(entityId) + sizeof(pos); }
		ULONGLONG entityId = 0;
		Vector3 pos{};
	};

	struct MakeOtherCharacterPacket : public PacketHeader
	{
		MakeOtherCharacterPacket() { type = MAKE_OTHER_CHARACTER_PACKET; size = sizeof(entityId) + sizeof(pos); }
		ULONGLONG entityId = 0;
		Vector3 pos{};
	};

	struct DeleteOtherCharacterPacket : public PacketHeader
	{
		DeleteOtherCharacterPacket() { type = DEL_OTHER_CHARACTER_PACKET; size = sizeof(entityId); }
		ULONGLONG entityId = 0;
	};
	struct GameLoadCompletePacket : public PacketHeader
	{
		GameLoadCompletePacket() { type = GAME_LOAD_COMPELTE_PACKET; }
	};

	// ���� GameStartPacket�� ���ư�.

	enum MoveDir : USHORT
	{
		LEFT = 0,
		LEFT_UP,
		UP,
		RIGHT_UP,
		RIGHT,
		RIGHT_DOWN,
		DOWN,
		LEFT_DOWN, 

		DIR_MAX,

		STOP = 65535,
	};
	struct MoveStartRequestPacket : public PacketHeader
	{
		MoveStartRequestPacket() { type = MOVE_START_REQUEST_PACKET; size = sizeof(pos) + sizeof(rotY); }
		Vector3 pos{};
		float rotY = 0; // rotX, rotZ �� 0�̴�.
	};

	struct MoveStartNotifyPacket : public PacketHeader
	{
		MoveStartNotifyPacket() { type = MOVE_START_NOTIFY_PACKET; size = sizeof(entityId) + sizeof(pos) + sizeof(rotY); }
		ULONGLONG entityId = 0;
		Vector3 pos{};
		float rotY = 0;
		//WORD moveDir = DIR_MAX;
	};

	struct MoveStopRequestPacket : public PacketHeader
	{
		MoveStopRequestPacket() { type = MOVE_STOP_REQUEST_PACKET; size = sizeof(pos) + sizeof(rotY); }
		Vector3 pos{};
		float rotY = 0;
	};

	struct MoveStopNotifyPacket : public PacketHeader
	{
		MoveStopNotifyPacket() { type = MOVE_STOP_NOTIFY_PACKET; size = sizeof(entityId) + sizeof(pos) + sizeof(rotY); }
		ULONGLONG entityId = 0;
		Vector3 pos{};
		float rotY = 0;
	};

	struct AttackRequestPacket : public PacketHeader
	{
		AttackRequestPacket() { type = ATTACK_REQUEST_PACKET;}
	};

	struct AttackNotifyPacket : public PacketHeader
	{
		AttackNotifyPacket() { type = ATTACK_NOTIFY_PACKET; size = sizeof(entityId); }
		ULONGLONG entityId = 0;
	};

	struct AttackedNotifyPacket : public PacketHeader
	{
		AttackedNotifyPacket() { type = ATTACKED_NOTIFY_PACKET; size = sizeof(entityId) + sizeof(currentHp); }
		ULONGLONG entityId = 0;
		USHORT currentHp = 0;
	};

	struct DieNotifyPacket : public PacketHeader
	{
		DieNotifyPacket() { type = DIE_NOTIFY_PACKET; size = sizeof(entityId); }
		ULONGLONG entityId = 0;
	};

	struct SpectatorInitPacket : public PacketHeader
	{
		SpectatorInitPacket() { type = SEPCTATOR_INIT_PACKET; }
	};

	struct GameEndNotifyPacket : public PacketHeader
	{
		GameEndNotifyPacket() { type = GAME_END_NOTIFY_PACKET; }
	};

	struct UpdateWinnerNotifyPacket : public PacketHeader
	{
		UpdateWinnerNotifyPacket() { type = UPDATE_WINNER_NOTIFY_PACKET; size = sizeof(userId) + sizeof(expectedTimeStamp); }
		ULONGLONG userId = 0;
		ULONGLONG expectedTimeStamp = 0;
	};

	struct InvalidateWinnerNotifyPacket : public PacketHeader
	{
		InvalidateWinnerNotifyPacket() { type = INVALIDATE_WINNER_NOTIFY_PACKET; size = sizeof(canceledUserId); }
		ULONGLONG canceledUserId = 0;
	};

	struct WinnerInfoNotifyPacket : public PacketHeader
	{
		WinnerInfoNotifyPacket() { type = WINNER_INFO_NOTIFY_PACKET; }
		// �ʿ信 ���� �߰�.
	};


	struct CharacterPositionSyncPacket : public PacketHeader
	{
		CharacterPositionSyncPacket() { type = CHARACTER_POSITION_SYNC_PACKET; size = sizeof(entityId) + sizeof(syncPos) + sizeof(syncRot); }
		ULONGLONG entityId = 0;
		Vector3 syncPos{};
		Vector3 syncRot{};
	};

	// ���� �ֱ�� �ڽ��� Position ����
	struct UpdateTransformPacket : public PacketHeader
	{
		UpdateTransformPacket() { type = UPDATE_TRANSFORM_PACKET; size = sizeof(timeStamp) + sizeof(entityId) + sizeof(pos) + sizeof(rot); }
		ULONGLONG timeStamp = 0;
		ULONGLONG entityId = 0;
		Vector3 pos{};
		Vector3 rot{};
	};
}

namespace jh_network
{
	// ---- HEART_BEAT
	struct HeartbeatPacket :public PacketHeader
	{
		HeartbeatPacket() { type = HEART_BEAT_PACKET; size = sizeof(timeStamp); }
		ULONGLONG timeStamp = 0;
	};
}

serializationBuffer& operator<< (serializationBuffer& serialBuffer,const Vector3& vector);
serializationBuffer& operator>> (serializationBuffer& serialBuffer,Vector3& vector);
// Only Has Head Packet
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::PacketHeader& packetHeader);

serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_content::RoomInfo& roomInfo);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_content::RoomInfo& roomInfo);
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::ErrorPacket& errorPacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::ErrorPacket& errorPacket);

// ���� ������ ��Ŷ�� ���� ������ �����ͱ����� �ֵ��� �Ѵ�.
// Packet ������ �� ��Ŷ�� �´� ����ȭ���� << operator�� ����������Ѵ�, PacketHeader�� ������ ����� �س��� ���¿����Ѵ�!  operator << >> �� �Է� / ��¸� ���� ���̴�.
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::LogInRequestPacket& logInRequestPacket);
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::LogInResponsePacket& logInResponsePacket);
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::MakeRoomRequestPacket& makeRoomRequestPacket);
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::MakeRoomResponsePacket& makeRoomResponsePacket);
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::EnterRoomResponsePacket& enterRoomResponsePacket);
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::EnterRoomNotifyPacket& enterRoomNotifyPacket); 
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::LeaveRoomNotifyPacket& leaveRoomNotifyPacket); 
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::OwnerChangeNotifyPacket& ownerChangeNotifyPacket); 
serializationBuffer& operator<< (serializationBuffer& serialBuffer, jh_network::GameReadyNotifyPacket& gameReadyNotifyPacket);

// >> opeartor ����, >> operator�� PacketHeader�� ���� �и��� �����߱⿡ packetHeader�� �����ʹ� �Ű澲�� �ʾƵ� �ȴ�.
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::LogInRequestPacket& logInRequestPacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::LogInResponsePacket& logInResponsePacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::MakeRoomRequestPacket& makeRoomRequestPacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::MakeRoomResponsePacket& makeRoomResponsePacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::EnterRoomNotifyPacket& enterRoomNotifyPacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::EnterRoomRequestPacket& enterRoomRequestPacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::LeaveRoomRequestPacket& leaveRoomRequestPacket);
serializationBuffer& operator>> (serializationBuffer& serialBuffer, jh_network::OwnerChangeNotifyPacket& ownerChangeNotifyPacket);

