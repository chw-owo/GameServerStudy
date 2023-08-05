#include "SystemLog.h"
#include <strsafe.h>
#include <iostream>
#include <tchar.h>
#include <time.h>

SystemLog* g_pSystemLog = SystemLog::GetInstance();

SystemLog::SystemLog(): _Dir(nullptr), _LogLevel(ERROR_LEVEL)
{
	
}

SystemLog* SystemLog::GetInstance()
{
	static SystemLog _systemLog;
	return &_systemLog;
}

void SystemLog::SetSysLogDir(const wchar_t* szDir)
{
	if (_Dir != nullptr)
		delete[] _Dir;

	int dirLen = wcslen(szDir);
	_Dir = new WCHAR[dirLen + 1];
	HRESULT ret = StringCchCopy(_Dir, dirLen + 1, szDir);
	_Dir[dirLen] = L'\0';

	if (FAILED(ret))
	{
		wprintf(L"Fail to StringCchCopy : %d (%s[%d])\n",
			ret, _T(__FUNCTION__), __LINE__);
		return;
	}

	CreateDirectory(_Dir, NULL);
}

void SystemLog::SetSysLogLevel(LOG_LEVEL LogLevel)
{
	_LogLevel = LogLevel;
}

void SystemLog::LogHex(const wchar_t* szType, LOG_LEVEL LogLevel, const wchar_t* szLog, BYTE* pByte, int iByteLen)
{
	if (LogLevel < _LogLevel) return;

	time_t baseTimer = time(NULL);
	struct tm localTimer;
	localtime_s(&localTimer, &baseTimer);

	WCHAR logTitle[dfTITLE_LEN] = { '\0' };
	HRESULT titleRet = StringCchPrintf(logTitle, dfTITLE_LEN, L"%s/%d%02d_%s_Binary.txt",
		_Dir, localTimer.tm_year + 1900, localTimer.tm_mon + 1, szType);
	logTitle[dfTITLE_LEN - 1] = L'\0';

	if (FAILED(titleRet))
	{
		wprintf(L"Fail to StringCchPrintf : %d (%s[%d])\n",
			titleRet, _T(__FUNCTION__), __LINE__);
		return;
	}

	WCHAR logData[dfDATA_LEN] = { '\0', };
	HRESULT copyRet = StringCchCopy(logData, dfDATA_LEN, szLog);
	if (FAILED(copyRet))
	{
		wprintf(L"Fail to StringCchCat : %d (%s[%d])\n",
			copyRet, _T(__FUNCTION__), __LINE__);
		return;
	}

	HRESULT concatRet1 = StringCchCat(logData, dfDATA_LEN, L"\n");
	if (FAILED(concatRet1))
	{
		wprintf(L"Fail to StringCchCat : %d (%s[%d])\n",
			concatRet1, _T(__FUNCTION__), __LINE__);
		return;
	}

	for (int i = 0; i < iByteLen; i++)
	{
		WCHAR logHexData[dfHEXDATA_LEN] = { '\0', };
		HRESULT printRet = StringCchPrintf(logHexData, dfHEXDATA_LEN, L"%02x ", pByte[i]);
		if (FAILED(printRet))
		{
			wprintf(L"Fail to StringCchPrintf : %d (%s[%d])\n",
				printRet, _T(__FUNCTION__), __LINE__);
			return;
		}

		HRESULT concatRet2 = StringCchCat(logData, dfDATA_LEN, logHexData);
		if (FAILED(concatRet2))
		{
			wprintf(L"Fail to StringCchCat : %d (%s[%d])\n",
				concatRet2, _T(__FUNCTION__), __LINE__);
			return;
		}
	}

	logData[dfDATA_LEN - 1] = L'\0';

	FILE* file;
	errno_t openRet = _wfopen_s(&file, logTitle, L"w");
	if (openRet != 0)
	{
		wprintf(L"Fail to open %s : %d (%s[%d])\n",
			logTitle, openRet, _T(__FUNCTION__), __LINE__);
		return;
	}

	if (file != nullptr)
	{
		fwprintf(file, logData);
		fclose(file);
	}
	else
	{
		wprintf(L"Fileptr is nullptr %s (%s[%d])\n",
			logTitle, _T(__FUNCTION__), __LINE__);
		return;
	}
}

void SystemLog::Log(const wchar_t* szType, LOG_LEVEL LogLevel, const wchar_t* szStringFormat, ...)
{
	if (LogLevel < _LogLevel) return;

	time_t baseTimer = time(NULL);
	struct tm localTimer;
	localtime_s(&localTimer, &baseTimer);

	WCHAR logTitle[dfTITLE_LEN] = { '\0' };
	HRESULT titleRet = StringCchPrintf(logTitle, dfTITLE_LEN, L"%s/%d%02d_%s.txt",
		_Dir, localTimer.tm_year + 1900, localTimer.tm_mon + 1, szType);
	logTitle[dfTITLE_LEN - 1] = L'\0';

	if (FAILED(titleRet))
	{
		wprintf(L"Fail to StringCchPrintf : %d (%s[%d])\n",
			titleRet, _T(__FUNCTION__), __LINE__);
		return;
	}
	 
	WCHAR logData[dfDATA_LEN] = { '\0', };
	va_list va;
	va_start(va, szStringFormat);
	HRESULT dataRet = StringCchVPrintf(logData, dfDATA_LEN, szStringFormat, va);
	logData[dfDATA_LEN - 1] = L'\0';
	va_end(va);

	if (FAILED(dataRet))
	{
		wprintf(L"Fail to StringCchVPrintf : %d (%s[%d])\n",
			dataRet, _T(__FUNCTION__), __LINE__);
		return;
	}

	FILE* file;
	errno_t openRet = _wfopen_s(&file, logTitle, L"w");
	if (openRet != 0)
	{
		wprintf(L"Fail to open %s : %d (%s[%d])\n", 
			logTitle, openRet, _T(__FUNCTION__), __LINE__);
		return;
	}
	
	if (file != nullptr)
	{
		fwprintf(file, logData);
		fclose(file);
	}
	else
	{
		wprintf(L"Fileptr is nullptr %s (%s[%d])\n", 
			logTitle, _T(__FUNCTION__), __LINE__);
		return;
	}
}
