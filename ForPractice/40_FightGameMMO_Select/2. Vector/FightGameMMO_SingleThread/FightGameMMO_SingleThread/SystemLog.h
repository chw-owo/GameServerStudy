#pragma once
#include <windows.h>
#define dfDATA_LEN 256
#define dfTITLE_LEN 256
#define dfHEXDATA_LEN 32

#define SYSTEM_LOG

class SystemLog
{
private:
	SystemLog();

public:
	static SystemLog* GetInstance();

public:
	enum LOG_LEVEL
	{
		DEBUG_LEVEL,
		ERROR_LEVEL,
		SYSTEM_LEVEL
	};

private:
	static SystemLog _systemLog;

public:
	void SetSysLogDir(const wchar_t* szDir);
	void SetSysLogLevel(LOG_LEVEL LogLevel);
	void Log(const wchar_t* szType, LOG_LEVEL LogLevel, const wchar_t* szStringFormat, ...);
	void LogHex(const wchar_t* szType, LOG_LEVEL LogLevel, const wchar_t* szLog, BYTE* pByte, int iByteLen);

private:
	WCHAR* _Dir = nullptr;
	LOG_LEVEL _LogLevel;
};

extern SystemLog* g_pSystemLog;

#ifdef SYSTEM_LOG
#define SYSLOG_DIRECTORY(DirName)						g_pSystemLog->SetSysLogDir(DirName)
#define SYSLOG_LEVEL(LogLevel)							g_pSystemLog->SetSysLogLevel(LogLevel)
#define LOG(LogType, LogLevel, Format, ...)				g_pSystemLog->Log(LogType, LogLevel, Format, ##__VA_ARGS__)
#define LOGHEX(LogType, LogLevel, Log, Byte, ByteLen)	g_pSystemLog->LogHex(LogType, LogLevel, Log, Byte, ByteLen)
#else
#define SYSLOG_DIRECTORY(DirName)
#define SYSLOG_LEVEL(LogLevel)
#define LOG(LogType, LogLevel, String, Input)
#define LOGHEX(LogType, LogLevel, Log, Byte, ByteLen) 
#endif