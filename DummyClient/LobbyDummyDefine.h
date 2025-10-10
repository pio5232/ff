#pragma once

#define LOBBY_DUMMY_FILE_NAME L"LobbyClient.cfg"
#define LOBBY_DUMMY_CATEGORY_NAME L"LobbyClient"

#define LOBBY_DUMMY_SAVEFILE_NAME L"LobbyClient"
#define LOGIC_THREAD_COUNT 5

enum class DummyStatus : byte
{
	DISCONNECTED = 0,
	SESSION_CONNECTED,
	IN_LOBBY,
	WAIT_ENTER_ROOM,
	IN_ROOM,
};
struct DummyData
{
	DummyData() : m_usRoomNum{}, m_wszRoomName{}, m_dummyStatus{ DummyStatus::DISCONNECTED }, m_ullSessionId{ (ULONGLONG)(INVALID_SESSION_ID) }, m_ullNextActionTime{}, m_ullLastUpdatedHeartbeatTime {} { aliveDummyCount.fetch_add(1); }
	~DummyData() { aliveDummyCount.fetch_sub(1); m_dummyStatus = DummyStatus::DISCONNECTED; }

	inline static std::atomic<int> aliveDummyCount = 0;
	USHORT m_usRoomNum;
	WCHAR m_wszRoomName[ROOM_NAME_MAX_LEN]{};
	DummyStatus m_dummyStatus;
	ULONGLONG m_ullSessionId;

	ULONGLONG m_ullNextActionTime;
	ULONGLONG m_ullLastUpdatedHeartbeatTime;
};

using DummyPtr = std::shared_ptr<DummyData>;

extern std::atomic<bool> reconnectFlag;
extern std::atomic<bool> clientSendFlag;

int GetRandValue(int div, int extra);
int GetRandTimeForDummy(); 