#pragma once

#define LOBBY_DUMMY_FILE_NAME L"LobbyClient.cfg"
#define LOBBY_DUMMY_CATEGORY_NAME L"LobbyClient"

#define LOBBY_DUMMY_SAVEFILE_NAME L"LobbyClient"
#define LOGIC_THREAD_COUNT 5

enum DummyStatus : byte
{
	DISCONNECTED = 0,
	SESSION_CONNECTED,
	IN_LOBBY,
	IN_ROOM,
};
struct DummyData
{
	USHORT roomNum;
	WCHAR roomName[ROOM_NAME_MAX_LEN]{};

};