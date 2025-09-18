#include "pch.h"
#include "LanClientSession.h"
#include "PacketHandler.h"
#include "PacketDefine.h"
#include "BufferMaker.h"
#include "GameServer.h"

jh_network::LanClientSession::LanClientSession(std::weak_ptr<jh_network::ServerBase> gameServer)
{
	if (gameServer.expired())
	{
		printf("게임 서버 Null임\n");
		_gameServer = std::weak_ptr<jh_network::GameServer>();
		return;
	}
	printf("게임 서버 NULL 아님\n");
	_gameServer = std::static_pointer_cast<jh_network::GameServer>(gameServer.lock());
}

void jh_network::LanClientSession::OnConnected()
{
	jh_network::GameServerSettingRequestPacket settingRequestPacket;

	jh_network::GameServerSettingRequestPacket requestPacket;

	SharedSendBuffer sendBuffer = jh_network::BufferMaker::MakePacket(requestPacket);

	Send(sendBuffer);

	//ULONGLONG xorAfterValue = _owner->GetToken() ^ xorTokenKey;

	//jh_network::GameServerInfoNotifyPacket packet;

	//jh_network::SharedSendBuffer buffer = jh_network::ChattingClientPacketHandler::MakeSendBuffer((sizeof(packet)));

	//const std::wstring ip = _owner->GetNetAddr().GetIpAddress();

	//uint16 port = _owner->GetNetAddr().GetPort();
	//if (0 == port)
	//	port = _owner->GetRealPort();

	//wcscpy_s(packet.ipStr, IP_STRING_LEN, ip.c_str());

	//*buffer << packet.size << packet.type;

	//buffer->PutData(reinterpret_cast<const char*>(packet.ipStr), IP_STRING_LEN * sizeof(WCHAR));

	//*buffer << port << _owner->GetRoomNumber() << xorAfterValue;

	//printf("ClientJoined Success\n");

	//wprintf(L"size : [ %d ]\n", packet.size);
	//wprintf(L"type : [ %d ]\n", packet.type);
	//wprintf(L"ip: [ %s ]\n", packet.ipStr);
	//wprintf(L"port : [ %d ]\n", port);
	//wprintf(L"Room : [ %d ]\n", _owner->GetRoomNumber());
	//wprintf(L"xorToken : [ %llu ], After : [%llu]\n", _owner->GetToken(), xorAfterValue);

	//Send(buffer);
}

void jh_network::LanClientSession::OnDisconnected()
{

}

void jh_network::LanClientSession::OnRecv(jh_utility::SerializationBuffer& buffer, uint16 type)
{
	LanClientSessionPtr sessionPtr = std::static_pointer_cast<jh_network::LanClientSession>(shared_from_this());

	if (ErrorCode::NONE != LanServerPacketHandler::ProcessPacket(sessionPtr, type, buffer))
	{
		// LOG
	}
}

std::shared_ptr<jh_network::GameServer> jh_network::LanClientSession::GetGameServer()
{
	if (_gameServer.expired())
		return nullptr;

	{
		std::shared_ptr<jh_network::GameServer> gameServer = _gameServer.lock();

		return gameServer;
	}
}

