#pragma once

#include <Windows.h>
#include <mutex>

#define LOG_WIDE2(x) L##x
#define LOG_WIDE1(x) LOG_WIDE2(x)

// 1차 폴더 (LOG)
#define BASE_DIR_PATH LOG_WIDE1(".\\FileLog")
// 사용은 DEFAULT_FILE_LOG_DIR_FILE_PATH 를 사용하도록 한다.
// 문자열 이어붙이기
#define ENTER_DIR(DIR) DIR L##"\\"
#define ADD_SPACE_DIR_PATH(DIR) DIR L##" "

#define DEFAULT_FILE_LOG_NAME L##"Log.txt"
// DEFAULT_FILE_LOG_DIR_FILE_PATH 길이보다 큰 최소값이어한다.
// 대략적으로 48보다 작아지면 파일 생성이 되지 않을 수 있음.

// 설정할 버퍼 크기 
#define DEFAULT_FILE_PATH_SIZE ((size_t)128)
#define DEFAULT_LOG_INFO_SIZE ((size_t)128)
#define DEFAULT_LOG_SIZE ((size_t)512)

// DEFAULT_FILE_PATH_SIZE 64 기준 약 15정도가 남음.
#define MAX_LOGTYPE_SIZE_INCLUDE_TXT (15+64)

namespace jh_utility
{

#define LOG_LEVEL_DEBUG jh_utility::FileLogger::LogLevel::LEVEL_DEBUG // 온갖 로그. 디버깅용
#define LOG_LEVEL_INFO jh_utility::FileLogger::LogLevel::LEVEL_INFO  
#define LOG_LEVEL_WARNING jh_utility::FileLogger::LogLevel::LEVEL_WARNING // 오류 로그
#define LOG_LEVEL_SYSTEM jh_utility::FileLogger::LogLevel::LEVEL_SYSTEM // 치명적 로그
#define LOG_LEVEL_NO_LOG jh_utility::FileLogger::LogLevel::LEVEL_NO_LOGGING

#define SET_LOG_LEVEL(LOG_LEVEL) g_logger->SetLogLevel(LOG_LEVEL)

#define _LOG(logType, logLevelMacro, format, ...)										\
do {																			\
	g_logger->WriteLog(logType, logLevelMacro, format L"\n", ##__VA_ARGS__);	\
} while(0)


	class FileLogger
	{
	public:
		enum class LogLevel : byte
		{
			LEVEL_DEBUG = 0,
			LEVEL_INFO,
			LEVEL_WARNING,
			LEVEL_SYSTEM,
			LEVEL_NO_LOGGING,
		};
;
		FileLogger();
		~FileLogger();

		// 로그 관련해서 데이터를 쓸 때는 예외가 발생하는 경우가 없도록 해야한다.
		// StringSafe한
		void WriteLog(const WCHAR* logType, LogLevel logLevel, const WCHAR* logFormat, ...);

		void SetLogLevel(LogLevel logLevel) { m_eLogLevel = logLevel; }

	private:

		struct LogInfo
		{

			WCHAR m_wchFilePath[DEFAULT_FILE_PATH_SIZE];
			WCHAR m_wchLogHeader[DEFAULT_LOG_INFO_SIZE];
			WCHAR m_wchLogMsg[DEFAULT_LOG_SIZE];
		};

		static unsigned WINAPI LogThreadMain(LPVOID lparam);
		void LogThreadFunc();

		bool CheckNCreateDir(const WCHAR* filePath);

		// 저장할 파일 이름,[type] [날짜 / LogLevel] 같은 정보들 세팅.
		// 무조건 성공해야하는 함수, 절대 실패할 수 없게 만들어야한다.
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
