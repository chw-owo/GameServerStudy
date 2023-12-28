#include "CLoginServer.h"
#include "CChattingServer.h"
#include "CMonitorClient.h"
#include "CCrashDump.h"
#include "CSystemLog.h"
#include <locale.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 

#define _MONITOR

CCrashDump g_Dump;
CLoginServer g_LoginServer;
CChattingServer g_ChattingServer;
CMonitorClient g_MonitorClient;

int wmain(int argc, wchar_t* argv[])
{
    _wsetlocale(LC_ALL, L"korean");

    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Start\n"); 

    timeBeginPeriod(1);
    if (!g_LoginServer.Initialize()) return 0;
    if (!g_ChattingServer.Initialize()) return 0;
    if (!g_MonitorClient.Initialize(&g_ChattingServer)) return 0;

    HANDLE event = CreateEvent(0, false, false, 0);
    if (event == 0) return 0;
    WaitForSingleObject(event, INFINITE);
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
    return 0;
}