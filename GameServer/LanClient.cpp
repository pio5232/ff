#include "pch.h"
#include "LanClient.h"
#include "GameServer.h"
#include "PacketHandler.h"

jh_network::LanClient::LanClient(const NetAddress& targetNetAddr, jh_network::SessionCreator creator)
	: ClientBase(targetNetAddr, creator)
{
	jh_network::LanServerPacketHandler::Init();
}

jh_network::LanClient::~LanClient()
{
}


