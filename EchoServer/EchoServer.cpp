#include "pch.h"
#include "EchoServer.h"
#include "EchoSystem.h"
#include "Memory.h"

jh_content::EchoServer::EchoServer() : IocpServer(ECHO_SERVER_SAVE_FILE_NAME)
{
	jh_utility::Parser parser;

	parser.LoadFile(UP_DIR(ECHO_SERVER_CONFIG_FILE));
	parser.SetReadingCategory(ECHO_CATEGORY_NAME);

	WCHAR ip[20];
	WORD port;
	WORD maxSessionCnt;
	DWORD concurrentWorkerThreadCount;

	WORD lingerOnOff;
	WORD lingerTime;
	ULONGLONG timeOut;

	bool succeeded = parser.GetValueWstr(L"serverIp", ip, ARRAY_SIZE(ip));
	succeeded &= parser.GetValue(L"serverPort", port);
	succeeded &= parser.GetValue(L"maxSessionCount", maxSessionCnt);
	succeeded &= parser.GetValue(L"concurrentWorkerThreadCount", concurrentWorkerThreadCount);

	succeeded &= parser.GetValue(L"lingerOnOff", lingerOnOff);
	succeeded &= parser.GetValue(L"lingerTime", lingerTime);
	succeeded &= parser.GetValue(L"TimeOut", timeOut);

	parser.CloseFile();

	if (true == succeeded)
		_LOG(L"ParseInfo", LOG_LEVEL_INFO, L"Parsing EchoServer is Completed [FileName : %s]", ECHO_SERVER_CONFIG_FILE);
	else
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"Parsing EchoServer is Failed...");
		jh_utility::CrashDump::Crash();
	}

	InitServerConfig(ip, port, concurrentWorkerThreadCount, lingerOnOff, lingerTime, timeOut);
	
	if (false == InitSessionArray(maxSessionCnt))
	{
		_LOG(L"ParseInfo", LOG_LEVEL_WARNING, L"[EchoServer()] - maxSession �ʱ�ȭ ����");
		jh_utility::CrashDump::Crash();
	}

	m_pEchoSystem = std::make_unique<EchoSystem>(this);
	
	m_pEchoSystem->Init();
}

jh_content::EchoServer::~EchoServer()
{
}

void jh_content::EchoServer::Monitor()
{
	wprintf(L" [Echo Server] Sessions : %d\n", GetSessionCount());
}

void jh_content::EchoServer::BeginAction()
{
}

void jh_content::EchoServer::EndAction()
{
}
//void jh_content::EchoServer::OnRecv(LONGLONG sessionId, jh_utility::SerializationBuffer* packet, WORD type)

void jh_content::EchoServer::OnRecv(ULONGLONG sessionId, PacketPtr packet, USHORT type)
{
	// ��Ƽ ������ ó�� ����
	short len;
	ULONGLONG data;

	*packet >> data;

	PacketPtr sendPacket = jh_content::PacketBuilder::BuildEchoPacket(8, data);
	SendPacket(sessionId, sendPacket);
	return;
	

	// ���� ������ ó�� ����.
	//JobPtr job = std::make_shared<jh_utility::Job>(sessionId, type, packet);
	JobPtr job = MakeShared<jh_utility::Job>(g_memAllocator, sessionId, type, packet);

	m_pEchoSystem->EnqueueJob(job);
}

void jh_content::EchoServer::OnConnected(ULONGLONG sessionId)
{
	//SessionConnectionEventPtr systemJob = std::make_shared<jh_utility::SessionConnectionEvent>(sessionId, jh_utility::SessionConnectionEventType::CONNECT);
	//SessionConnectionEventPtr sessionConnectionEventPtr = MakeShared<jh_utility::SessionConnectionEvent>(g_memAllocator, sessionId, jh_utility::SessionConnectionEventType::CONNECT);

	//m_pEchoSystem->EnqueueSystemJob(sessionConnectionEventPtr);
}
void jh_content::EchoServer::OnDisconnected(ULONGLONG sessionId)
{
	//SessionConnectionEventPtr systemJob = std::make_shared<jh_utility::SessionConnectionEvent>(sessionId, jh_utility::SessionConnectionEventType::DISCONNECT);
	//SessionConnectionEventPtr sessionConnectionEventPtr = MakeShared<jh_utility::SessionConnectionEvent>(g_memAllocator, sessionId, jh_utility::SessionConnectionEventType::DISCONNECT); // MakeSystemJob(sessionId, jh_utility::SessionConnectionEventType::DISCONNECT);


	//m_pEchoSystem->EnqueueSystemJob(sessionConnectionEventPtr);
}
