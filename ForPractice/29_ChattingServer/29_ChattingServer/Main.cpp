#include "Main.h"
#include "NetworkManager.h"
#include "ContentManager.h"

bool g_bShutdown = false;

int main()
{
	NetworkManager* networkManager = NetworkManager::GetInstance();
	ContentManager* contentManager = ContentManager::GetInstance();

	while (!g_bShutdown)
	{
		networkManager->Update();
		contentManager->Update();
	}

	return 0;

}