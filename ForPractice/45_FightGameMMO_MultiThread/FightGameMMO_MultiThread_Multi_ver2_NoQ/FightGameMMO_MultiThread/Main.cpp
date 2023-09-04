#include "Main.h"
#include "CGameServer.h"

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 
bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{ 
    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Start\n");

    timeBeginPeriod(1);
    CGameServer* pGameServer = CGameServer::GetInstance();
    while (!g_bShutdown)
    {
        if (GetAsyncKeyState(VK_ESCAPE))
            pGameServer->NetworkTerminate();
    }
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Terminate\n");
    return 0;
}