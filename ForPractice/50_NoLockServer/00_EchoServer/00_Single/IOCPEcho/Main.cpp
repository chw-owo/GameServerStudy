#include "CEchoServer.h"
#include "CSystemLog.h"

CEchoServer g_Server;
int wmain(int argc, wchar_t* argv[])
{
	SYSLOG_DIRECTORY(L"SystemLog");
	SYSLOG_LEVEL(CSystemLog::DEBUG_LEVEL);
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Start\n");

	g_Server.Initialize();
	for(;;)
	{
		Sleep(1000);
		/*
		if (GetAsyncKeyState(VK_SPACE))
		{
			g_Server.Terminate();
			break;
		}
		*/
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Thread Terminate\n");
	return 0;
}