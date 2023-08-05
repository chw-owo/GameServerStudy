#include "Main.h"

bool g_bShutdown = false;
int wmain(int argc, wchar_t* argv[])
{
	timeBeginPeriod(1);

	Server* server = Server::GetInstance();
	while (!g_bShutdown)
	{
		server->NetworkUpdate();
		server->ContentUpdate();
		server->Monitor();
		// server->Control();
	}

	timeEndPeriod(1);
	return 0;
}