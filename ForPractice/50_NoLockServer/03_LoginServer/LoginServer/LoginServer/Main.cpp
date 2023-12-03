#include "CLoginServer.h"
#include "CChattingServer.h"
#include "CCrashDump.h"
#include "CSystemLog.h"
#include <locale.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 

CCrashDump g_Dump;
CLoginServer g_LoginServer;
CChattingServer g_ChattingServer;

int wmain(int argc, wchar_t* argv[])
{
    _wsetlocale(LC_ALL, L"korean");

    SYSLOG_DIRECTORY(L"SystemLog");
    SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Start\n");

    timeBeginPeriod(1);
    if (!g_LoginServer.Initialize()) return 0;
    if (!g_ChattingServer.Initialize()) return 0;

    for (;;)
    {
        // if (GetAsyncKeyState(VK_NUMPAD1)) g_LoginServer.Terminate();
        // if (GetAsyncKeyState(VK_NUMPAD2)) g_ChattingServer.Terminate();
    }
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
    return 0;
}