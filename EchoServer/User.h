#pragma once

namespace jh_content
{
	class User
	{

	public:
		User(ULONGLONG userId) : _userId(userId), _isReady(false), _isJoinedRoom(false),
			_roomNum(0)
		{
			InterlockedIncrement64((LONGLONG*)&_aliveLobbyUserCount);
		}

		~User()
		{
			InterlockedDecrement64((LONGLONG*)&_aliveLobbyUserCount);
		}


		// Resulta
		const std::pair<bool, USHORT> GetRoomId() const;
		ULONGLONG GetUserId() const { return _userId; }
	public:
		ULONGLONG _userId;
		bool _isReady;

	private:
		USHORT _roomNum;
		bool _isJoinedRoom;
		static alignas(64) volatile ULONGLONG _aliveLobbyUserCount;

		// 나갈땐 invalid로 설정.
	};
}
