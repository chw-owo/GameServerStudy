#include "Server.h"
#include "Main.h"

bool g_bShutdown = false;

int main()
{
	Server* server = Server::GetInstance();
	while (!g_bShutdown)
	{
		server->Update();
	}
	return 0;
}