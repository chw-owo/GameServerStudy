#include "CLanMonitorServer.h"
#include "CNetMonitorServer.h"
#include "CCrashDump.h"
#include "CSystemLog.h"
#include <locale.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 
CLanMonitorServer g_LanMonitorServer; // Recv Data from Server, Save DB
CNetMonitorServer g_NetMonitorServer; // Send to Monitor Program

int wmain(int argc, wchar_t* argv[])
{
    _wsetlocale(LC_ALL, L"korean");

    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Start\n");

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    ::wprintf(L"CPU total: %d\n\n", si.dwNumberOfProcessors);

    timeBeginPeriod(1);

    if (!g_LanMonitorServer.Initialize()) return 0;
    if (!g_NetMonitorServer.Initialize()) return 0;

    HANDLE event = CreateEvent(0, false, false, 0);
    if (event == 0) return 0;
    WaitForSingleObject(event, INFINITE); 
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
    return 0;
}
