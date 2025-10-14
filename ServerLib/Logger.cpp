#include "LibraryPch.h"
#include <ctime>
#include <strsafe.h>
#include "Logger.h"
#include "Memory.h"
//jh_utility::FileLogger* jh_utility::FileLogger::m_pInstance = nullptr;
//std::once_flag jh_utility::FileLogger::m_onceFlag{};


// ���� �� �޸� ���� ������, �α״� ���������� ���� �ϱ⿡ ���д�.
//jh_utility::FileLogger& jh_utility::FileLogger::GetInstance()
//{
//	static FileLogger instance;
//	//std::call_once(m_onceFlag, []() {m_pInstance = new FileLogger(); 		});
//
//	return instance;
//}

jh_utility::FileLogger::FileLogger() : m_ullLogCounter(0), m_wszCommonFilePath{}, m_bIsRunning{ 1 }, m_bCasOpen{ true }
{
	SetLogLevel(LOG_LEVEL_SYSTEM);

	// �ð� ���� ����ü
	tm localTime;
	time_t utcTime;
	DWORD outPathLen;
	DWORD outFileLen;

	// GetUTCTime 
	time(&utcTime);
	localtime_s(&localTime, &utcTime);

	int len = swprintf_s(m_wszCommonFilePath, DEFAULT_FILE_PATH_SIZE, ENTER_DIR(BASE_DIR_PATH) L"%04d %02d %02d %02d_%02d_%02d", localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
		localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

	if (true == CheckNCreateDir(BASE_DIR_PATH) && true == CheckNCreateDir(m_wszCommonFilePath))
	{
		if (SUCCEEDED(StringCchCat(&m_wszCommonFilePath[len], sizeof(m_wszCommonFilePath) / sizeof(m_wszCommonFilePath[0]) - len, L"\\")))
		{
			wprintf(L"m_wszCommonFilePath : [%s]\n", m_wszCommonFilePath);
		}
		else
		{
			wprintf(L"FileLogger() StringCchCat(\"\\\\\") is Failed..\n");
			CrashDump::Crash();
		}
	}

	m_hLogEvent = CreateEvent(nullptr, false, false, nullptr);
	
	if (nullptr == m_hLogEvent)
		CrashDump::Crash();

	m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LogThreadMain, this, 0, nullptr));

	if (nullptr == m_hThread)
		CrashDump::Crash();

	return;
}

unsigned __stdcall jh_utility::FileLogger::LogThreadMain(LPVOID lparam)
{
	FileLogger* instance = static_cast<FileLogger*>(lparam);
	
	instance->LogThreadFunc();

	return 0;
}

void jh_utility::FileLogger::LogThreadFunc()
{
	std::queue<LogInfo*> logQ;
	
	while (1 == m_bIsRunning)
	{
		WaitForSingleObject(m_hLogEvent, INFINITE);

		m_logQ.Swap(logQ);

		while (logQ.size() > 0)
		{
			LogInfo* logInfo = logQ.front();

			logQ.pop();

			if (true == m_bCasOpen)
			{
				FILE* file;

				errno_t fileErrorCode = _wfopen_s(&file, logInfo->m_pPath, L"a+, ccs=UNICODE");

				if (0 != fileErrorCode || nullptr == file)
				{
					m_bCasOpen = false;
				}
				else
				{
					fwprintf_s(file, L"[logCounter : %20llu] %s %s\n", ++m_ullLogCounter, logInfo->m_pHeader, logInfo->m_pMsg);

					fclose(file);
				}
			}

			g_memAllocator->Free(logInfo->m_pPath);
			g_memAllocator->Free(logInfo->m_pHeader);
			g_memAllocator->Free(logInfo->m_pMsg);

			g_memAllocator->Free(logInfo);
		}
	}
}

bool jh_utility::FileLogger::CheckNCreateDir(const WCHAR* filePath)
{
	DWORD AttributesInfo = GetFileAttributes(filePath);

	if (INVALID_FILE_ATTRIBUTES == AttributesInfo)
	{
		DWORD GetFileAttributesError = GetLastError();

		bool isCreated = CreateDirectory(filePath, nullptr);

		if (false == isCreated)
		{
			DWORD CreateDirectoryError = GetLastError();
			wprintf(L"CheckNCreateDir is Failed GetFileAttributes Error : [%u], CreateDirectory Error : [%u], Path : [%s]\n", GetFileAttributesError, CreateDirectoryError, filePath);
			return false;
		}
		return true;
	}

	// ���� Ư�� ����� ���� ������ ��ũ ������.
	// https://learn.microsoft.com/ko-kr/windows/win32/fileio/file-attribute-constants
	// �ش� ������ Directory�� �ƴ� ��
	if (0 == (FILE_ATTRIBUTE_DIRECTORY & AttributesInfo))
	{
		wprintf(L"CheckNCreateDir - FILE_ATTRIBUTE is Not DIRECTORY\n");
		return false;
	}

	return true;
}

jh_utility::FileLogger::~FileLogger()
{
	if (InterlockedExchange8(&m_bIsRunning, 0) == 1)
	{
		SetEvent(m_hLogEvent);
		
		WaitForSingleObject(m_hThread, INFINITE);
		
		CloseHandle(m_hThread);
		CloseHandle(m_hLogEvent);
	}

	std::queue<LogInfo*> remainingLogInfoQ;
	m_logQ.Swap(remainingLogInfoQ);

	while (remainingLogInfoQ.size() > 0)
	{
		LogInfo* logInfo = remainingLogInfoQ.front();
		
		g_memAllocator->Free(logInfo->m_pPath);
		g_memAllocator->Free(logInfo->m_pHeader);
		g_memAllocator->Free(logInfo->m_pMsg);

		remainingLogInfoQ.pop();

		g_memAllocator->Free(logInfo);
	}

	//if (nullptr != m_pInstance)
	//{
	//	delete m_pInstance;
	//	m_pInstance = nullptr;
	//}
}

void jh_utility::FileLogger::WriteLog(const WCHAR* logType, LogLevel logLevel, const WCHAR* logFormat, ...)
{
	if (m_eLogLevel > logLevel)
		return;

	WCHAR* filePathBuffer = static_cast<WCHAR*>(g_memAllocator->Alloc(DEFAULT_FILE_PATH_SIZE * sizeof(WCHAR)));
	WCHAR* logHeaderBuffer = static_cast<WCHAR*>(g_memAllocator->Alloc(DEFAULT_LOG_INFO_SIZE * sizeof(WCHAR)));
	WCHAR* logBodyBuffer = static_cast<WCHAR*>(g_memAllocator->Alloc(DEFAULT_LOG_SIZE * sizeof(WCHAR)));

	const size_t maxFileNameBufferSize = DEFAULT_FILE_PATH_SIZE;// sizeof(filePathBuffer) / sizeof(filePathBuffer[0]);
	const size_t maxLogInfoBuffeSize = DEFAULT_LOG_INFO_SIZE; // sizeof(logInfoBuffer) / sizeof(logInfoBuffer[0]);

	static_assert(maxFileNameBufferSize >= sizeof(m_wszCommonFilePath) / sizeof(m_wszCommonFilePath[0]));

	SetLogInfo(filePathBuffer, maxFileNameBufferSize, logType, logLevel, logHeaderBuffer, maxLogInfoBuffeSize);

	va_list args;
	va_start(args, logFormat);
	HRESULT hResult = StringCchVPrintf(logBodyBuffer, DEFAULT_LOG_SIZE, logFormat, args);
	va_end(args);
	
	if (FAILED(hResult))
	{
		StringCchPrintf(logBodyBuffer, DEFAULT_LOG_SIZE, L" [ WriteLog - LogFormat Size > DEFAULT_LOG_SIZE ]\n");
	}

	LogInfo* logInfo = static_cast<LogInfo*>(g_memAllocator->Alloc(sizeof(LogInfo)));

	logInfo->m_pPath = filePathBuffer;
	logInfo->m_pHeader = logHeaderBuffer;
	logInfo->m_pMsg = logBodyBuffer;

	m_logQ.Push(logInfo);
	SetEvent(m_hLogEvent);
	//wprintf(L"[WRITE LOG] \nFileName : [%s]\nLogInfo : %s\nLog : %s\n", filePathBuffer, logHeaderBuffer, logBodyBuffer);

}

//void jh_utility::FileLogger::WriteLogHex(const WCHAR* logType, LogLevel logLevel, const WCHAR* logWstr, BYTE* byteBuffer, int byteBufferLen)
//{
//	if (m_eLogLevel > logLevel)
//		return;
//
//	FILE* file;
//
//	alignas(64) WCHAR filePathBuffer[DEFAULT_FILE_PATH_SIZE]{};
//	alignas(64) WCHAR logInfoBuffer[DEFAULT_LOG_INFO_SIZE]{};
//
//	const DWORD logBufferSize = DEFAULT_LOG_SIZE * 4;
//
//	// [BYTE ] ���� ���ۿ� �־���ϱ� ������ ���۸� ũ�� ��´�.
//	alignas(64) WCHAR logBuffer[logBufferSize]{};
//
//	const size_t maxFilePathBufferSize = sizeof(filePathBuffer) / sizeof(filePathBuffer[0]);
//	const size_t maxLogInfoBuffeSize = sizeof(logInfoBuffer) / sizeof(logInfoBuffer[0]);
//
//	static_assert(maxFilePathBufferSize >= sizeof(m_wszCommonFilePath) / sizeof(m_wszCommonFilePath[0]));
//
//	SetLogInfo(filePathBuffer, maxFilePathBufferSize, logType, logLevel, logInfoBuffer, maxLogInfoBuffeSize);
//
//	{
//		HRESULT hResult = StringCchPrintf(logBuffer, logBufferSize, logWstr);
//
//		if (FAILED(hResult))
//		{
//			StringCchPrintf(logBuffer, logBufferSize, L" [ WriteLogHex - LogFormat Size > DEFAULT_LOG_SIZE ]\n");
//		}
//	}
//
//	for (int i = 0; i < byteBufferLen; i++)
//	{
//		// stringCchCató�� �̾���̱�
//		size_t offset = wcslen(logBuffer);
//
//		HRESULT hResult = StringCchPrintf(&logBuffer[offset], logBufferSize - offset, L" %02x", byteBuffer[i]);
//	
//		if (FAILED(hResult))
//		{
//			StringCchPrintf(logBuffer, logBufferSize, L" [ WriteLogHex - LogFormat Size > DEFAULT_LOG_SIZE ]\n");
//			break;
//		}
//	}
//
//	{
//		// �α׸� ���� ������ open / close
//		// �����̵� ���е� �α״� ����Ǿ���Ѵ�.
//		SRWLockGuard lockGuard(&m_logQLock);
//
//		errno_t fileErrorCode = _wfopen_s(&file, fileNameBuffer, L"a+, ccs=UNICODE");
//
//		if (0 != fileErrorCode || nullptr == file)
//		{
//			DWORD get = GetLastError();
//			wprintf(L"file open is failed!!\n");
//			return;
//		}
//		fwprintf_s(file, L"%s ", logInfoBuffer);
//		fwprintf_s(file, L"%s\n", logBuffer);
//
//		fclose(file);
//	}
//
//	wprintf(L"[WRITE LOG HEX] FileName : [%s], LogInfo : %s , Log : %s\n", fileNameBuffer, logInfoBuffer, logBuffer);
//
//}


void jh_utility::FileLogger::SetLogInfo(WCHAR* filePath, size_t filePathBufferSize, const WCHAR* logType, LogLevel logLevel, WCHAR* logInfoBuffer, size_t maxLogInfoBufferSize)
{
	// [�ð�logtype.txt] 
	DWORD logTypeNameLen = wcslen(logType) + wcslen(L".txt") + 1;
	size_t offset = wcslen(m_wszCommonFilePath);

	StringCchCopy(filePath, filePathBufferSize, m_wszCommonFilePath);

	////[�ð�logType.txt] �߰��� ���� ������ ������ default
	if (logTypeNameLen > MAX_LOGTYPE_SIZE_INCLUDE_TXT)
	{
		StringCchCat(&filePath[offset], filePathBufferSize - offset, DEFAULT_FILE_LOG_NAME);
	}
	else
	{
		StringCchCat(&filePath[offset], filePathBufferSize - offset, logType);
		offset += wcslen(logType);

		StringCchCat(&filePath[offset], filePathBufferSize - offset, L".txt");
	}

	// DIR + FILE + TYPE.txt ���� �Ϸ�.
	tm localTime;
	time_t utcTime;
	// GetUTCTime 
	time(&utcTime);
	localtime_s(&localTime, &utcTime);

	// Log������ ����.����
	// [��¥ / logLevel] 
	WCHAR levelWstr[10]{};

	switch (logLevel)
	{
	case LogLevel::LEVEL_DETAIL:
		wcscpy_s(levelWstr, L"DETAIL"); break;
	case LogLevel::LEVEL_DEBUG:
		wcscpy_s(levelWstr, L"DEBUG"); break;
	case LogLevel::LEVEL_INFO:
		wcscpy_s(levelWstr, L"INFO"); break;
	case LogLevel::LEVEL_WARNING:
		wcscpy_s(levelWstr, L"WARNING"); break;
	case LogLevel::LEVEL_SYSTEM:
		wcscpy_s(levelWstr, L"SYSTEM"); break;
	default:
		break;
	}

	DWORD threadID = GetCurrentThreadId();
	HRESULT hRes = StringCchPrintf(logInfoBuffer, maxLogInfoBufferSize, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d / %-8s] [ThreadID : %6u] ",
		logType, localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
		localTime.tm_hour, localTime.tm_min, localTime.tm_sec, levelWstr, threadID);

	if (FAILED(hRes))
	{
		// ���� ũ��
		StringCchPrintf(logInfoBuffer, maxLogInfoBufferSize, L"[LogType Truncated : Too Large] [%04d-%02d-%02d %02d:%02d:%02d / %-8s] [ThreadID : %6u]",
			localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
			localTime.tm_hour, localTime.tm_min, localTime.tm_sec, levelWstr, threadID);
	}
}
