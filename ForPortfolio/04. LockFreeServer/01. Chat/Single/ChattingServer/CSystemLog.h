#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#define dfDATA_LEN 1024
#define dfINFO_LEN 256
#define dfTITLE_LEN 256
#define dfHEXDATA_LEN 32

#define __SYSTEM_LOG
#ifdef __SYSTEM_LOG

#define __SYSTEM_LOG_MULTITHREAD

class CSystemLog
{
private:
	CSystemLog();

public:
	static CSystemLog* GetInstance();

public:
	enum LOG_LEVEL
	{
		DEBUG_LEVEL,
		ERROR_LEVEL,
		SYSTEM_LEVEL
	};

private:
	static CSystemLog _systemLog;

public:
	void SetSysLogDir(const wchar_t* dir);
	void SetSysLogLevel(LOG_LEVEL logLevel);
	void Log(const wchar_t* type, LOG_LEVEL logLevel, const wchar_t* stringFormat, ...);
	void LogHex(const wchar_t* type, LOG_LEVEL logLevel, const wchar_t* log, BYTE* pByte, int byteLen);

private:
	WCHAR* _dir = nullptr;
	LOG_LEVEL _logLevel;
	__int64 _logCount;

#ifdef  __SYSTEM_LOG_MULTITHREAD
	CRITICAL_SECTION _cs;
#endif
};

extern CSystemLog* g_pSystemLog;
#endif

#ifdef __SYSTEM_LOG
#define SYSLOG_DIRECTORY(dirName)						g_pSystemLog->SetSysLogDir(dirName)
#define SYSLOG_LEVEL(logLevel)							g_pSystemLog->SetSysLogLevel(logLevel)
#define LOG(logType, logLevel, format, ...)				g_pSystemLog->Log(logType, logLevel, format, ##__VA_ARGS__)
#define LOGHEX(logType, logLevel, log, pByte, byteLen)	g_pSystemLog->LogHex(logType, logLevel, log, pByte, byteLen)
#else
#define SYSLOG_DIRECTORY(dirName)
#define SYSLOG_LEVEL(logLevel)
#define LOG(logType, logLevel, format, ...)
#define LOGHEX(logType, logLevel, log, byte, byteLen) 
#endif