#pragma once

#include <MSWSock.h>

namespace jh_network
{
	/*--------------------
		  NetAddress
	--------------------*/
	class NetAddress
	{
	public:
		static void Init();
		static void Clear();

		NetAddress(SOCKADDR_IN sockAddr);
		NetAddress(std::wstring ip, USHORT port);
		NetAddress(const NetAddress& other);
		NetAddress() {}

		NetAddress& operator=(const NetAddress& other);
		NetAddress& operator=(const SOCKADDR_IN& sockAddr);

		void Reset() { _sockAddr = {}; }
		void Init(SOCKADDR_IN sockAddr);
		const SOCKADDR_IN& GetSockAddr() const { return _sockAddr; }
		const std::wstring	GetIpAddress() const;
		const USHORT GetPort() const { return ntohs(_sockAddr.sin_port); }

		static IN_ADDR IpToAddr(const WCHAR* ip);
		static USHORT GetPort(SOCKET sock);

		static LPFN_CONNECTEX lpfnConnectEx;
	private:
		SOCKADDR_IN _sockAddr = {};
	};
}
