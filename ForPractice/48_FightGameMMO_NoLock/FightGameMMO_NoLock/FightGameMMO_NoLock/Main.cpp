
/*
<TO-DO>
1. 콘텐츠는 싱글 스레드로
2. NetLib은 락프리 구조로
4. 프로파일러 동적 STL로 수정
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
    for(;;)
    {
        if (GetAsyncKeyState(VK_ESCAPE))
            g_GameServer.Terminate();
    }
    timeEndPeriod(1);

    LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
    return 0;
}

