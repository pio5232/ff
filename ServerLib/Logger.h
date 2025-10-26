#pragma once

#include <Windows.h>
#include <mutex>

#define LOG_WIDE2(x) L##x
#define LOG_WIDE1(x) LOG_WIDE2(x)

// 1�� ���� (LOG)
#define BASE_DIR_PATH LOG_WIDE1(".\\FileLog")
// ����� DEFAULT_FILE_LOG_DIR_FILE_PATH �� ����ϵ��� �Ѵ�.
// ���ڿ� �̾���̱�
#define ENTER_DIR(DIR) DIR L##"\\"
#define ADD_SPACE_DIR_PATH(DIR) DIR L##" "

#define DEFAULT_FILE_LOG_NAME L##"Log.txt"
// DEFAULT_FILE_LOG_DIR_FILE_PATH ���̺��� ū �ּҰ��̾��Ѵ�.
// �뷫������ 48���� �۾����� ���� ������ ���� ���� �� ����.

// ������ ���� ũ�� 
#define DEFAULT_FILE_PATH_SIZE 128
#define DEFAULT_LOG_INFO_SIZE 128
#define DEFAULT_LOG_SIZE 1024

// DEFAULT_FILE_PATH_SIZE 64 ���� �� 15������ ����.
#define MAX_LOGTYPE_SIZE_INCLUDE_TXT (15+64)

namespace jh_utility
{

#define LOG_LEVEL_DETAIL jh_utility::FileLogger::LogLevel::LEVEL_DETAIL
#define LOG_LEVEL_DEBUG jh_utility::FileLogger::LogLevel::LEVEL_DEBUG // �°� �α�. ������
#define LOG_LEVEL_INFO jh_utility::FileLogger::LogLevel::LEVEL_INFO  // ������ �α�
#define LOG_LEVEL_WARNING jh_utility::FileLogger::LogLevel::LEVEL_WARNING // ���� �α�
#define LOG_LEVEL_SYSTEM jh_utility::FileLogger::LogLevel::LEVEL_SYSTEM // ġ���� �α�
#define LOG_LEVEL_NO_LOG jh_utility::FileLogger::LogLevel::LEVEL_NO_LOGGING

//#define SET_LOG_LEVEL(LOG_LEVEL) jh_utility::FileLogger::GetInstance().SetLogLevel(LOG_LEVEL)
#define SET_LOG_LEVEL(LOG_LEVEL) g_logger->SetLogLevel(LOG_LEVEL)

/* jh_utility::FileLogger::GetInstance().WriteLog(logType, logLevelMacro, format L"\n", ##__VA_ARGS__);	\ */
#define _LOG(logType, logLevelMacro, format, ...)										\
do {																			\
	g_logger->WriteLog(logType, logLevelMacro, format L"\n", ##__VA_ARGS__);	\
} while(0)

/*jh_utility::FileLogger::GetInstance().WriteLogHex(logType, logLevelMacro, logWstr, buffer, bufferSize);	\*/
//#define _LOGHEX(logType, logLevelMacro, logWstr, buffer, bufferSize)											\
//do {																				\
//	g_logger->WriteLogHex(logType, logLevelMacro, logWstr, buffer, bufferSize);	\
//} while(0)

	class FileLogger
	{
	public:
		enum class LogLevel : byte
		{
			LEVEL_DETAIL= 0,
			LEVEL_DEBUG,
			LEVEL_INFO,
			LEVEL_WARNING,
			LEVEL_SYSTEM,
			LEVEL_NO_LOGGING,
		};
;
		//static FileLogger& GetInstance();

		FileLogger();
		~FileLogger();

		// �α� �����ؼ� �����͸� �� ���� ���ܰ� �߻��ϴ� ��찡 ������ �ؾ��Ѵ�.
		// StringSafe��
		void WriteLog(const WCHAR* logType, LogLevel logLevel, const WCHAR* logFormat, ...);

		// ���̳ʸ� �����͸� Hex 16������ ����� �Լ�. 
		//void WriteLogHex(const WCHAR* logType, LogLevel logLevel, const WCHAR* logWstr, BYTE* buffer, int bufferLen);

		void SetLogLevel(LogLevel logLevel) { m_eLogLevel = logLevel; }

	private:

		struct LogInfo
		{
			WCHAR* m_pPath;
			WCHAR* m_pHeader;
			WCHAR* m_pMsg;
		};

		static unsigned WINAPI LogThreadMain(LPVOID lparam);
		void LogThreadFunc();

		bool CheckNCreateDir(const WCHAR* filePath);

		// ������ ���� �̸�,[type] [��¥ / LogLevel] ���� ������ ����.
		// ������ �����ؾ��ϴ� �Լ�, ���� ������ �� ���� �������Ѵ�.
		void SetLogInfo(WCHAR* fileNameBuffer, size_t fileNameBufferSize, const WCHAR* logType, LogLevel logLevel, WCHAR* logInfoBuffer, size_t logInfoBufferSize);

		// .\\FileLog\\DATE_TIME
		WCHAR m_wszCommonFilePath[DEFAULT_FILE_PATH_SIZE];

		LogLevel m_eLogLevel;

		jh_utility::LockQueue<LogInfo*> m_logQ;

		ULONGLONG m_ullLogCounter;

		volatile char m_bIsRunning;
		bool m_bCasOpen;
		HANDLE m_hThread;
		HANDLE m_hLogEvent;
	};
}
