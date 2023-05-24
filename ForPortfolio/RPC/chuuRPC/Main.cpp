#include "GameServer.h"

int main(int argc, char* argv[])
{
	GameServer* server = GameServer::GetInstance();

	server->Initialize();
	for(;;)
		server->Update();
	server->Terminate();

	return 0;
}