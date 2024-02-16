#include "CGameServer.h"

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 
CGameServer g_GameServer;

int wmain(int argc, wchar_t* argv[])
{ 
    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Start\n");

    timeBeginPeriod(1);
    g_GameServer.Initialize();

    Sleep(INFINITE);

    // TO-DO
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Terminate\n");
    return 0;
}