﻿#include "Main.h"

bool g_bShutdown = false;
int wmain(int argc, wchar_t* argv[])
{
	SYSLOG_DIRECTORY(L"SystemLog");
	SYSLOG_LEVEL(SystemLog::ERROR_LEVEL);
	LOG(L"SYSTEM", SystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Start\n");

	Server* server = Server::GetInstance();
	while (!g_bShutdown)
	{
		server->NetworkUpdate();
		server->ContentUpdate();
		server->Monitor();
		// server->Control();
	}

	LOG(L"SYSTEM", SystemLog::SYSTEM_LEVEL, L"%s", L"Main Thread Terminate\n");
	return 0;
}