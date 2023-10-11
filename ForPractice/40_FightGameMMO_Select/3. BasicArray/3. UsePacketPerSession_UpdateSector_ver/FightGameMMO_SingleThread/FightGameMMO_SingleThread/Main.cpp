#include "Main.h"

bool g_bShutdown = false;
int wmain(int argc, wchar_t* argv[])
{
	SYSLOG_DIRECTORY(L"SystemLog");
	SYSLOG_LEVEL(SystemLog::ERROR_LEVEL);
	LOG(L"SYSTEM", SystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Start\n");

	Server* pServer = new Server;
	while (!g_bShutdown)
	{
		pServer->NetworkUpdate();
		pServer->ContentUpdate();
		pServer->Monitor();
	}
	delete pServer;
	LOG(L"SYSTEM", SystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Terminate\n");
	return 0;
}