/*
<TO-DO>
1. 서버는 문제 없는데 클라에서 Wrong Disconnect 발생
*/

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

