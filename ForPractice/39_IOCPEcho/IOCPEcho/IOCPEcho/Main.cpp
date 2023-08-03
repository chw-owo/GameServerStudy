#include "Main.h"

bool g_bShutdown = false;
int wmain(int argc, wchar_t* argv[])
{
	Server* server = Server::GetInstance();
	while (!g_bShutdown)
	{
		server->Monitor();
	}
	return 0;
}