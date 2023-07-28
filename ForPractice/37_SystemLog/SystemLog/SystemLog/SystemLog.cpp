#include "SystemLog.h"

SystemLog* SystemLog::GetInstance()
{
	static SystemLog _systemLog;
	return &_systemLog;
}

void SystemLog::Log(WCHAR* szType, LOG_LEVEL LogLevel, WCHAR* szStringFormat, ...)
{
	if (_LogLevel == DEBUG_LEVEL)
	{

	}
	else if (_LogLevel == ERROR_LEVEL)
	{

	}
	else if (_LogLevel == SYSTEM_LEVEL)
	{

	}
}

void SystemLog::LogHex(WCHAR* szType, LOG_LEVEL LogLevel, WCHAR* szLog, BYTE* pByte, int iByteLen)
{

}

void SystemLog::SetSysLogDir(WCHAR* szDir)
{
}

void SystemLog::SetSysLogLevel(LOG_LEVEL LogLevel)
{
}
