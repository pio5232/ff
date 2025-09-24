#include "LibraryPch.h"
#include "NetworkUtils.h"
#include <WS2tcpip.h>
#include "cassert"
/*--------------------
	  NetAddress
--------------------*/

LPFN_CONNECTEX jh_network::NetAddress::lpfnConnectEx = nullptr;

void jh_network::NetAddress::Init()
{
	WSAData wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		jh_utility::CrashDump::Crash();

	SOCKET dummy = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	
	if(INVALID_SOCKET == dummy)
		jh_utility::CrashDump::Crash();

	GUID guid = WSAID_CONNECTEX;
	DWORD byteRet = 0;

	WSAIoctl(dummy, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &lpfnConnectEx, sizeof(lpfnConnectEx), &byteRet, nullptr, nullptr);

	closesocket(dummy);
}

void jh_network::NetAddress::Clear()
{
	WSACleanup();
}

jh_network::NetAddress::NetAddress(SOCKADDR_IN sockAddr) :_sockAddr(sockAddr) {}


jh_network::NetAddress::NetAddress(std::wstring ip, USHORT port)
{
	memset(&_sockAddr, 0, sizeof(_sockAddr));

	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_port = htons(port);
	_sockAddr.sin_addr = IpToAddr(ip.c_str());
}

jh_network::NetAddress::NetAddress(const NetAddress& other) : _sockAddr(other._sockAddr) {}

jh_network::NetAddress& jh_network::NetAddress::operator=(const NetAddress& other)
{
	_sockAddr = other._sockAddr;

	return *this;
}

jh_network::NetAddress& jh_network::NetAddress::operator=(const SOCKADDR_IN& other)
{
	_sockAddr = other;

	return *this;
}

void jh_network::NetAddress::Init(SOCKADDR_IN sockAddr)
{
	_sockAddr = sockAddr;
}

const std::wstring jh_network::NetAddress::GetIpAddress() const
{
	WCHAR ipWstr[IP_STRING_LEN]{};

	// �ּ�ü��, &IN_ADDR
	InetNtopW(AF_INET, &_sockAddr.sin_addr, ipWstr, sizeof(ipWstr) / sizeof(WCHAR));

	return std::wstring(ipWstr);
}

IN_ADDR jh_network::NetAddress::IpToAddr(const WCHAR* ip)
{
	IN_ADDR addr;
	InetPtonW(AF_INET, ip, &addr);
	return addr;
}

USHORT jh_network::NetAddress::GetPort(SOCKET sock) 
{
	if (sock == INVALID_SOCKET)
	{
		return 0;
	}

	SOCKADDR_IN addr;
	int addrLen = sizeof(addr);

	if (getsockname(sock, (SOCKADDR*)&addr, &addrLen) == SOCKET_ERROR)
	{
		return 0;
	}

	return ntohs(addr.sin_port);
}