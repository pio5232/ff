#include "LibraryPch.h"
#include <ctime>
#include <strsafe.h>
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

jh_utility::FileLogger::FileLogger() : m_llLogCounter(0), m_wszCommonFilePath{}
{
	SetLogLevel(LOG_LEVEL_SYSTEM);
	InitializeSRWLock(&m_lock);

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

			return;
		}
		wprintf(L"FileLogger() StringCchCat(\"\\\\\") is Failed..\n");
		CrashDump::Crash();
	}

	return;
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

	FILE* file;

	alignas(64) WCHAR filePathBuffer[DEFAULT_FILE_PATH_SIZE]{};
	alignas(64) WCHAR logInfoBuffer[DEFAULT_LOG_INFO_SIZE]{};
	alignas(64) WCHAR logBuffer[DEFAULT_LOG_SIZE]{};

	const size_t maxFileNameBufferSize = sizeof(filePathBuffer) / sizeof(filePathBuffer[0]);
	const size_t maxLogInfoBuffeSize = sizeof(logInfoBuffer) / sizeof(logInfoBuffer[0]);

	static_assert(maxFileNameBufferSize >= sizeof(m_wszCommonFilePath) / sizeof(m_wszCommonFilePath[0]));

	SetLogInfo(filePathBuffer, maxFileNameBufferSize, logType, logLevel, logInfoBuffer, maxLogInfoBuffeSize);

	va_list args;
	va_start(args, logFormat);
	HRESULT hResult = StringCchVPrintf(logBuffer, DEFAULT_LOG_SIZE, logFormat, args);
	va_end(args);
	;
	if (FAILED(hResult))
	{
		StringCchPrintf(logBuffer, DEFAULT_LOG_SIZE, L" [ WriteLog - LogFormat Size > DEFAULT_LOG_SIZE ]\n");
	}

	{
		// �α׸� ���� ������ open / close
		// �����̵� ���е� �α״� ����Ǿ���Ѵ�.
		SRWLockGuard lockGuard(&m_lock);

		errno_t fileErrorCode = _wfopen_s(&file, filePathBuffer, L"a+, ccs=UNICODE");

		if (0 != fileErrorCode || nullptr == file)
		{
			DWORD get = GetLastError();
			wprintf(L"file open is failed!!\n");
			return;
		}
		fwprintf_s(file, L"%s ", logInfoBuffer);
		fwprintf_s(file, L"%s\n", logBuffer);

		fclose(file);
	}

	//wprintf(L"[WRITE LOG] \nFileName : [%s]\nLogInfo : %s\nLog : %s\n", filePathBuffer, logInfoBuffer, logBuffer);

}

void jh_utility::FileLogger::WriteLogHex(const WCHAR* logType, LogLevel logLevel, const WCHAR* logWstr, BYTE* byteBuffer, int byteBufferLen)
{
	if (m_eLogLevel < logLevel)
		return;

	FILE* file;

	alignas(64) WCHAR fileNameBuffer[DEFAULT_FILE_PATH_SIZE]{};
	alignas(64) WCHAR logInfoBuffer[DEFAULT_LOG_INFO_SIZE]{};

	const DWORD logBufferSize = DEFAULT_LOG_SIZE * 4;

	// [BYTE ] ���� ���ۿ� �־���ϱ� ������ ���۸� ũ�� ��´�.
	alignas(64) WCHAR logBuffer[logBufferSize]{};

	const size_t maxFileNameBufferSize = sizeof(fileNameBuffer) / sizeof(fileNameBuffer[0]);
	const size_t maxLogInfoBuffeSize = sizeof(logInfoBuffer) / sizeof(logInfoBuffer[0]);

	static_assert(maxFileNameBufferSize >= sizeof(m_wszCommonFilePath) / sizeof(m_wszCommonFilePath[0]));

	SetLogInfo(fileNameBuffer, maxFileNameBufferSize, logType, logLevel, logInfoBuffer, maxLogInfoBuffeSize);

	{
		HRESULT hResult = StringCchPrintf(logBuffer, logBufferSize, logWstr);

		if (FAILED(hResult))
		{
			StringCchPrintf(logBuffer, logBufferSize, L" [ WriteLogHex - LogFormat Size > DEFAULT_LOG_SIZE ]\n");
		}
	}

	for (int i = 0; i < byteBufferLen; i++)
	{
		// stringCchCató�� �̾���̱�
		size_t offset = wcslen(logBuffer);

		HRESULT hResult = StringCchPrintf(&logBuffer[offset], logBufferSize - offset, L" %02x", byteBuffer[i]);

		if (FAILED(hResult))
		{
			StringCchPrintf(logBuffer, logBufferSize, L" [ WriteLogHex - LogFormat Size > DEFAULT_LOG_SIZE ]\n");
			break;
		}
	}

	{
		// �α׸� ���� ������ open / close
		// �����̵� ���е� �α״� ����Ǿ���Ѵ�.
		SRWLockGuard lockGuard(&m_lock);

		errno_t fileErrorCode = _wfopen_s(&file, fileNameBuffer, L"a+, ccs=UNICODE");

		if (0 != fileErrorCode || nullptr == file)
		{
			DWORD get = GetLastError();
			wprintf(L"file open is failed!!\n");
			return;
		}
		fwprintf_s(file, L"%s ", logInfoBuffer);
		fwprintf_s(file, L"%s\n", logBuffer);

		fclose(file);
	}

	wprintf(L"[WRITE LOG HEX] FileName : [%s], LogInfo : %s , Log : %s\n", fileNameBuffer, logInfoBuffer, logBuffer);

}


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

	ULONGLONG logIndex = InterlockedIncrement64(&m_llLogCounter);

	DWORD threadID = GetCurrentThreadId();
	HRESULT hRes = StringCchPrintf(logInfoBuffer, maxLogInfoBufferSize, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d / %-8s] [logCounter : %20llu] [ThreadID : %6u] ",
		logType, localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
		localTime.tm_hour, localTime.tm_min, localTime.tm_sec, levelWstr, logIndex, threadID);

	if (FAILED(hRes))
	{
		// ���� ũ��
		StringCchPrintf(logInfoBuffer, maxLogInfoBufferSize, L"[LogType Truncated : Too Large] [%04d-%02d-%02d %02d:%02d:%02d / %-8s] [logCounter : %20llu] [ThreadID : %6u]",
			localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
			localTime.tm_hour, localTime.tm_min, localTime.tm_sec, levelWstr, logIndex, threadID);
	}
}
