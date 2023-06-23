#include "Main.h"
#include "NetworkManager.h"
#include "ContentManager.h"

bool g_bShutdown = false;

int main()
{
	NetworkManager* pNetworkManager = NetworkManager::GetInstance();
	ContentManager* pContentManager = ContentManager::GetInstance();

	while (!g_bShutdown)
	{
		pNetworkManager->Update();
		pContentManager->Update();
	}

	return 0;

}