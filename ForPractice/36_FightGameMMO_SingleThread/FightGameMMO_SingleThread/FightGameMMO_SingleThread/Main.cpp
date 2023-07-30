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
		// server->Control();
		// server->Monitor();
	}

	timeEndPeriod(1);
	return 0;
}