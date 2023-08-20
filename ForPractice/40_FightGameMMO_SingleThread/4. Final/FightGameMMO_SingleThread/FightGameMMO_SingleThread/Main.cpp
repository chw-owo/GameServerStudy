#include "Main.h"
#include "Server.h"
#include "SystemLog.h"

bool g_Shutdown = false;
int wmain(int argc, wchar_t* argv[])
{
	SYSLOG_DIRECTORY(L"SystemLog");
	SYSLOG_LEVEL(CSystemLog::ERROR_LEVEL);
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Start\n");

	CServer* pServer = new CServer;

	while (!g_Shutdown)
	{
		if (GetAsyncKeyState(VK_SPACE)) break;
		pServer->NetworkUpdate();
		pServer->ContentUpdate();
		pServer->MonitorUpdate();
	}
	delete pServer;
	
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Terminate\n");
	return 0;
}