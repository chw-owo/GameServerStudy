﻿#include "CMonitorServer.h"
#include "CCrashDump.h"
#include "CSystemLog.h"
#include <locale.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 
CMonitorServer g_MonitorServer;

int wmain(int argc, wchar_t* argv[])
{
    _wsetlocale(LC_ALL, L"korean");

    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Start\n");

    timeBeginPeriod(1);
    if (!g_MonitorServer.Initialize()) return 0;
    for (;;)
    {
        // if (GetAsyncKeyState(VK_NUMPAD1)) g_MonitorServer.Terminate();
    }
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
    return 0;
}
