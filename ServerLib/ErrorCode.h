#pragma once

// AA NetErrorCode ��ġ�� �ٽ� ����
enum class ErrorCode : WORD 
{
	NONE = 0, // ����

	// [ 1 ~ NETWORK ] 
	WSA_START_UP_ERROR,

	CREATE_COMPLETION_PORT_FAILED,
	CREATE_SOCKET_FAILED,
	LISTEN_FAILED,
	BIND_FAILED,
	SET_NET_ADDR_FAILED, // NET ADDR ���� ����.
	SET_SOCK_OPT_FAILED,

	CLIENT_NOT_INITIALIZED,
	CLIENT_CONNECT_FAILED,
	CLIENT_NOT_CONNECTED,

	RECV_BUF_OVERFLOW, // ���� ���۰� ������ų �� �ִ� ũ�⺸�� ũ�� transferredBytes�� �� �� �߻���.
	RECV_BUF_DEQUE_FAILED, // ���� �Ͼ�� �ȵǴ� ����. �߻����� ���� ������ ���踦 Ȯ���ؾ���.

	SEND_LEN_ZERO, // 0 

	// [ 30000 ~ CONTENT ]
	CREATE_ROOM_FAILED,
	MAX_ROOM,
	ACCESS_DESTROYED_ROOM, // ������ room�� ����
	CANNOT_FIND_ROOM, // roomNum���� room�� ã�� ����. �Ǵ� User�� Room�� ������ �ִµ� �� Room�� �������� �ʴ� ����
	ALREADY_EXIST_ROOM, // �����ϸ� �ȵǴµ�.. room�� �����ϴ� ����.

	ALLOC_FAILED,	// malloc, new �� �޸� �Ҵ� ����

	DUPLICATED_MEMBER, // Dic�� ����Ϸ��� �ߴµ� �̹� �����ΰ��� ����� �Ǿ� ����.
	ACCESS_DELETE_MEMBER, // ������ ����� ����.


	POST_UI_UPDATE_FAILED, // UITaskManager�� postMessage�� �����ߴ�.
	SERVER_IS_NOT_EXIST, // GAME_SERVER�� �������� ����.
	GAME_SERVER_IS_RUNNING, // ���� ���� ���� ��û�� ������. �̹� ���� ���� ���� ( ��û�� �ߺ� üũ )
	// [ 60000 ~ ETC / COMMON]
	NOT_FOUND = 60000, // ����� ����.
	CANNOT_FIND_PACKET_FUNC, // PacketHandler�� �ش� ��Ŷ�� ó���� �� �ִ� �Լ��� ��ϵ��� ����.

	WRONG_TOKEN,
};