#include "Main.h"
#include "Server.h"
#include "SystemLog.h"

CCrashDump dump;

int wmain(int argc, wchar_t* argv[])
{
	SYSLOG_DIRECTORY(L"SystemLog");
	SYSLOG_LEVEL(CSystemLog::DEBUG_LEVEL);
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Start\n");

	CServer* pServer = new CServer;
	for(;;)
	{
		pServer->NetworkUpdate();
		pServer->ContentUpdate();
		pServer->MonitorUpdate();
	}
	delete pServer;
	
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Terminate\n");
	return 0;
}