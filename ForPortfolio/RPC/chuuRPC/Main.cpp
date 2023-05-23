#include "Main.h"

bool g_bShutdown = false;
int main(int argc, char* argv[])
{
	GameServer* server = GameServer::GetInstance();
	server->Initialize();
	while (!g_bShutdown)
	{
		server->Update();
	}
	server->Terminate();
	return 0;
}