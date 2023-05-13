#include "Main.h"

bool g_bShutdown = false;
int main(int argc, char* argv[])
{
	NetworkManager* networkMgr = NetworkManager::GetInstance();
	PlayerManager* playerMgr = PlayerManager::GetInstance();

	networkMgr->Initialize();

	while (!g_bShutdown)
	{
		networkMgr->Update();
		playerMgr->Update();
	}

	networkMgr->Terminate();
	playerMgr->Terminate();

	return 0;
}
