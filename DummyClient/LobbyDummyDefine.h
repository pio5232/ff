#pragma once

#define LOBBY_DUMMY_FILE_NAME L"LobbyClient.cfg"
#define LOBBY_DUMMY_CATEGORY_NAME L"LobbyClient"

#define LOBBY_DUMMY_SAVEFILE_NAME L"LobbyClient"
#define LOGIC_THREAD_COUNT 5

#define RE_SEND_TIMEOUT ((int)8000)
enum class DummyStatus : USHORT
{
	DISCONNECTED =			1 << 0,
	SESSION_CONNECTED =		1 << 1,
	IN_LOBBY =				1 << 2,
	WAIT_ENTER_ROOM =		1 << 3,
	IN_ROOM =				1 << 4 ,
	IN_ROOM_WAIT_CHAT =		1 << 5,
	WAIT_LEAVE_ROOM =		1 << 6,
	CHECK_RTT =				1 << 7,
	NO_CHANGE =				1 << 8,
};
struct DummyData
{
	DummyData() : m_usExpectedRoomNum{}, m_wszExpectedRoomName{}, m_dummyStatus{ DummyStatus::DISCONNECTED }, m_ullSessionId{ (ULONGLONG)(INVALID_SESSION_ID) }, m_ullNextActionTime{}, m_ullLastUpdatedHeartbeatTime{}, m_ullUserId{}, m_ullLastSendTime{} { InterlockedIncrement(&aliveDummyCount); }
	~DummyData() { InterlockedDecrement(&aliveDummyCount); m_dummyStatus = DummyStatus::DISCONNECTED; m_ullUserId = 0; }

	USHORT m_usExpectedRoomNum;
	WCHAR m_wszExpectedRoomName[ROOM_NAME_MAX_LEN]{};
	DummyStatus m_dummyStatus;
	ULONGLONG m_ullSessionId;
	ULONGLONG m_ullUserId;

	ULONGLONG m_ullNextActionTime;
	ULONGLONG m_ullLastUpdatedHeartbeatTime;
	ULONGLONG m_ullLastSendTime;

	static alignas(64) LONG aliveDummyCount;
};

using DummyPtr = std::shared_ptr<DummyData>;

extern std::atomic<bool> reconnectFlag;
extern std::atomic<bool> clientSendFlag;

int GetRandValue(int div, int extra);
int GetRandTimeForDummy(); 