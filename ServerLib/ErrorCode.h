#pragma once

// AA NetErrorCode 합치고 다시 정리
enum class ErrorCode : WORD 
{
	NONE = 0, // 정상

	// [ 1 ~ NETWORK ] 
	WSA_START_UP_ERROR,

	CREATE_COMPLETION_PORT_FAILED,
	CREATE_SOCKET_FAILED,
	LISTEN_FAILED,
	BIND_FAILED,
	SET_NET_ADDR_FAILED, // NET ADDR 설정 실패.
	SET_SOCK_OPT_FAILED,

	CLIENT_NOT_INITIALIZED,
	CLIENT_CONNECT_FAILED,
	CLIENT_NOT_CONNECTED,

	RECV_BUF_OVERFLOW, // 수신 버퍼가 증가시킬 수 있는 크기보다 크게 transferredBytes가 올 때 발생함.
	RECV_BUF_DEQUE_FAILED, // 절대 일어나면 안되는 에러. 발생했을 때는 링버퍼 설계를 확인해야함.

	SEND_LEN_ZERO, // 0 

	// [ 30000 ~ CONTENT ]
	CREATE_ROOM_FAILED,
	MAX_ROOM,
	ACCESS_DESTROYED_ROOM, // 삭제된 room에 접근
	CANNOT_FIND_ROOM, // roomNum으로 room을 찾지 못함. 또는 User가 Room을 가지고 있는데 그 Room이 존재하지 않는 상태
	ALREADY_EXIST_ROOM, // 존재하면 안되는데.. room이 존재하는 상태.

	ALLOC_FAILED,	// malloc, new 등 메모리 할당 실패

	DUPLICATED_MEMBER, // Dic에 등록하려고 했는데 이미 무엇인가가 등록이 되어 있음.
	ACCESS_DELETE_MEMBER, // 삭제된 멤버에 접근.


	POST_UI_UPDATE_FAILED, // UITaskManager에 postMessage가 실패했다.
	SERVER_IS_NOT_EXIST, // GAME_SERVER가 존재하지 않음.
	GAME_SERVER_IS_RUNNING, // 게임 서버 시작 요청을 했지만. 이미 실행 중인 상태 ( 요청의 중복 체크 )
	// [ 60000 ~ ETC / COMMON]
	NOT_FOUND = 60000, // 멤버가 없음.
	CANNOT_FIND_PACKET_FUNC, // PacketHandler에 해당 패킷을 처리할 수 있는 함수가 등록되지 않음.

	WRONG_TOKEN,
};