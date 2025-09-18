#include "pch.h"
#include "GameServer.h"
#include "PacketHandler.h"
#include "PacketDefine.h"
#include "UserManager.h"
#include "BufferMaker.h"
#include "GameSession.h"
#include "LanClientSession.h"
#include "GamePlayer.h"
#include "WorldChat.h"

std::unordered_map<uint16, jh_network::GameClientPacketHandler::PacketFunc> jh_network::GameClientPacketHandler::_packetFuncsDic;
std::unordered_map<uint16, jh_network::LanServerPacketHandler::PacketFunc> jh_network::LanServerPacketHandler::_packetFuncsDic;


// ----------------------------- GameClientPacketHandler ------------------

void jh_network::GameClientPacketHandler::Init()
{
	_packetFuncsDic.clear();

	_packetFuncsDic[ENTER_GAME_REQUEST_PACKET] = ProcessEnterGameRequestPacket;
	_packetFuncsDic[GAME_LOAD_COMPELTE_PACKET] = ProcessLoadCompletedPacket;

	_packetFuncsDic[MOVE_START_REQUEST_PACKET] = ProcessMoveStartRequestPacket;
	_packetFuncsDic[MOVE_STOP_REQUEST_PACKET] = ProcessMoveStopRequestPacket;
	_packetFuncsDic[ATTACK_REQUEST_PACKET] = ProcessAttackRequestPacket;

	_packetFuncsDic[CHAT_TO_ROOM_REQUEST_PACKET] = ProcessChatRequestPacket;

}

ErrorCode jh_network::GameClientPacketHandler::ProcessEnterGameRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	std::shared_ptr<GameServer> server = std::static_pointer_cast<GameServer>(gameSessionPtr->GetServer());
	//jh_network::EnterGameRequestPacket packet;
	ULONGLONG userId;
	ULONGLONG token;
	buffer >> userId >> token;
	printf("Enter Game Packet Recv.. %llu, userId : %llu\n", gameSessionPtr->GetSessionId(), userId);

	ULONGLONG serverToken = server->GetToken();

	//SharedSession session = _sessionMgr->GetSession(sessionId);

	if (serverToken != token)
	{
		printf("WRONG TOKEN!!\n");
		jh_network::SharedSendBuffer sharedBuffer = jh_network::BufferMaker::MakeErrorPacket(jh_network::PacketErrorCode::CONNECTED_FAILED_WRONG_TOKEN);

		gameSessionPtr->Send(sharedBuffer);

		gameSessionPtr->Disconnect();
		return ErrorCode::WRONG_TOKEN;
	}

	gameSessionPtr->Init(userId);
	// 플레이어 입장.
	GamePlayerPtr myPlayer = server->CreatePlayer(gameSessionPtr);
	gameSessionPtr->SetPlayer(myPlayer);

	jh_network::EnterGameResponsePacket enterGameResponsePacket;

	jh_network::SharedSendBuffer sharedBuffer = jh_network::BufferMaker::MakePacket(enterGameResponsePacket);

	gameSessionPtr->Send(sharedBuffer);

	jh_network::MakeMyCharacterPacket makeMyCharacterPacket;
	makeMyCharacterPacket.entityId = myPlayer->GetEntityId();
	makeMyCharacterPacket.pos = myPlayer->GetPosition();

	jh_network::SharedSendBuffer makeMyCharacterBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(makeMyCharacterPacket));

	*makeMyCharacterBuffer << makeMyCharacterPacket.size << makeMyCharacterPacket.type <<
		makeMyCharacterPacket.entityId << makeMyCharacterPacket.pos;

	gameSessionPtr->Send(makeMyCharacterBuffer);

	server->TryRun();

	return ErrorCode::NONE;
}

ErrorCode jh_network::GameClientPacketHandler::ProcessLoadCompletedPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	printf("OnLoadComplet %llu\n", gameSessionPtr->GetUserId());

	std::shared_ptr<jh_network::GameServer> gameServer = std::static_pointer_cast<jh_network::GameServer>(gameSessionPtr->GetServer());

	gameSessionPtr->OnLoadComplete();

	gameServer->CheckLoadingAndStartLogic();

	return ErrorCode();
}

ErrorCode jh_network::GameClientPacketHandler::ProcessMoveStartRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	jh_network::MoveStartRequestPacket packet;
	buffer >> packet.pos >> packet.rotY;

	std::shared_ptr<jh_network::GameServer> gameServer = std::static_pointer_cast<jh_network::GameServer>(gameSessionPtr->GetServer());

	GameSessionPtr myGSession = gameSessionPtr;
	gameServer->EnqueueAction([packet, myGSession]()
		{
			GamePlayerPtr gamePlayer = myGSession->GetPlayer();

			if (gamePlayer->IsDead())
				return;

			gamePlayer->ProcessMoveStartPacket(packet);
		});
	//if(gamePlayerPtr->_transformComponent._position.)

	return ErrorCode::NONE;
}

ErrorCode jh_network::GameClientPacketHandler::ProcessMoveStopRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	jh_network::MoveStopRequestPacket packet;
	buffer >> packet.pos >> packet.rotY;

	std::shared_ptr<jh_network::GameServer> gameServer = std::static_pointer_cast<jh_network::GameServer>(gameSessionPtr->GetServer());

	GameSessionPtr myGSession = gameSessionPtr;

	gameServer->EnqueueAction([packet, myGSession]()
		{
			GamePlayerPtr gamePlayer = myGSession->GetPlayer();

			if (gamePlayer->IsDead())
				return;

			gamePlayer->ProcessMoveStopPacket(packet);
		});

	return ErrorCode::NONE;
}

ErrorCode jh_network::GameClientPacketHandler::ProcessChatRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	// GameWorld Chat
	// roomNum은 무시하도록 한다.
	uint16 roomNum;
	uint16 messageLen;

	buffer >> roomNum >> messageLen;

	char* payLoad = static_cast<char*>(malloc(messageLen));

	buffer.GetData(payLoad, messageLen);

	ULONGLONG userId = gameSessionPtr->GetUserId();

	PacketHeader packetHeader;

	// --- NotifyPacket
	packetHeader.size = sizeof(userId) + sizeof(messageLen) + messageLen;
	packetHeader.type = CHAT_NOTIFY_PACKET;

	jh_network::SharedSendBuffer notifyBuffer = jh_network::BufferMaker::MakeSendBuffer(sizeof(packetHeader) + packetHeader.size);

	*notifyBuffer << packetHeader << userId << messageLen;
	notifyBuffer->PutData(reinterpret_cast<const char*>(payLoad), messageLen);

	free(payLoad);

	WorldChatPtr worldChatPtr = std::static_pointer_cast<jh_network::GameServer>(gameSessionPtr->GetServer())->GetWorldChatPtr();

	worldChatPtr->DoAsync(&jh_content::WorldChat::Chat, notifyBuffer);

	return ErrorCode::NONE;
}

ErrorCode jh_network::GameClientPacketHandler::ProcessAttackRequestPacket(GameSessionPtr& gameSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	jh_network::AttackRequestPacket packet;

	std::shared_ptr<jh_network::GameServer> gameServer = std::static_pointer_cast<jh_network::GameServer>(gameSessionPtr->GetServer());

	gameServer->ProxyAttackPacket(gameSessionPtr, packet);

	return ErrorCode::NONE;
}

// --------------------------- LanServerPacketHandler ---------------------
void jh_network::LanServerPacketHandler::Init()
{
	_packetFuncsDic.clear();

	_packetFuncsDic[GAME_SERVER_SETTING_RESPONSE_PACKET] = ProcessGameServerSettingResponsePacket;
}

ErrorCode jh_network::LanServerPacketHandler::ProcessGameServerSettingResponsePacket(LanClientSessionPtr& lanClientSessionPtr, jh_utility::SerializationBuffer& buffer)
{
	GameServerSettingResponsePacket responsePacket;

	uint16 roomNum;
	uint16 requiredUsers;
	uint16 maxUsers;

	buffer >> roomNum >> requiredUsers >> maxUsers;

	printf("방 번호 [%d]\n", roomNum);
	printf("인원	[%d]\n", requiredUsers);
	printf("제한 인원  [%d]\n", maxUsers);

	std::shared_ptr<GameServer> gameServer = lanClientSessionPtr->GetGameServer();//lanClientSessionPtr->GetGameServer();

	if (gameServer == nullptr)
	{
		// LOG
		printf("GameServer is Not Exist\n");
		return ErrorCode::SERVER_IS_NOT_EXIST;
	}

	// G
	gameServer->Init(roomNum, requiredUsers, maxUsers);

	ULONGLONG xorAfterValue = gameServer->GetToken() ^ xorTokenKey;

	jh_network::GameServerLanInfoPacket packet;

	jh_network::SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakeSendBuffer((sizeof(packet)));

	const std::wstring ip = gameServer->GetNetAddr().GetIpAddress();

	uint16 port = gameServer->GetNetAddr().GetPort();
	if (0 == port)
		port = gameServer->GetRealPort();

	wcscpy_s(packet.ipStr, IP_STRING_LEN, ip.c_str());

	*sendBuffer << packet.size << packet.type;

	sendBuffer->PutData(reinterpret_cast<const char*>(packet.ipStr), IP_STRING_LEN * sizeof(WCHAR));

	*sendBuffer << port << gameServer->GetRoomNumber() << xorAfterValue;

	printf("ClientJoined Success\n");

	wprintf(L"size : [ %d ]\n", packet.size);
	wprintf(L"type : [ %d ]\n", packet.type);
	wprintf(L"ip: [ %s ]\n", packet.ipStr);
	wprintf(L"port : [ %d ]\n", port);
	wprintf(L"Room : [ %d ]\n", gameServer->GetRoomNumber());
	wprintf(L"xorToken : [ %llu ], After : [%llu]\n", gameServer->GetToken(), xorAfterValue);

	lanClientSessionPtr->Send(sendBuffer);
	// 
	// 
	//gameServer->Begin();

	return ErrorCode::NONE;
}

