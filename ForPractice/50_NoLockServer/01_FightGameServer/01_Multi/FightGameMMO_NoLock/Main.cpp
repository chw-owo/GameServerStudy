#include "CGameServer.h"
#include "CCrashDump.h"
#include "CSystemLog.h"

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 

CCrashDump g_Dump;
CGameServer g_GameServer;

int wmain(int argc, wchar_t* argv[])
{
    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::DEBUG_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Start\n");

    timeBeginPeriod(1);
    g_GameServer.Initialize();
    for(;;)
    {
        if (GetAsyncKeyState(VK_SPACE))
            g_GameServer.Terminate();
    }
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
    return 0;
}

