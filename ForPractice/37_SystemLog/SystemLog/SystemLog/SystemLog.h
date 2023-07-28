#pragma once
#include <Windows.h>

#define SYSTEM_LOG

#ifdef SYSTEM_LOG
#define SYSLOG_DIRECTORY(DirName) SystemLog::GetInstance->SetSysLogDir(DirName)
#define SYSLOG_LEVEL(LogLevel) SystemLog::GetInstance->SetSysLogLevel(LogLevel)
#define LOG(Input) SystemLog::GetInstance->Log(Input)
#else
#define SYSLOG_DIRECTORY(DirName)
#define SYSLOG_LEVEL(LogLevel)
#endif

class SystemLog
{
private:
	SystemLog();

public:	
	static SystemLog* GetInstance();

private:
	static SystemLog _systemLog;

public:
	enum LOG_LEVEL
	{
		DEBUG_LEVEL,
		ERROR_LEVEL,
		SYSTEM_LEVEL
	};

	void SetSysLogDir(WCHAR* szDir);
	void SetSysLogLevel(LOG_LEVEL LogLevel);
	void Log(WCHAR* szType, LOG_LEVEL LogLevel, WCHAR* szStringFormat, ...);
	void LogHex(WCHAR* szType, LOG_LEVEL LogLevel, WCHAR* szLog, BYTE* pByte, int iByteLen);

private:
	WCHAR* _Dir;
	LOG_LEVEL _LogLevel;
};
