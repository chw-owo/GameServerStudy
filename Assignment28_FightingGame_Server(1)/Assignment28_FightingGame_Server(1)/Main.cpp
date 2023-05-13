#include "Main.h"

DWORD g_oldTick;
bool g_bShutdown = false;
int main(int argc, char* argv[])
{
	timeBeginPeriod(1);
	g_oldTick = timeGetTime();
	NetworkManager* networkMgr = NetworkManager::GetInstance();
	PlayerManager* playerMgr = PlayerManager::GetInstance();

	networkMgr->Initialize();

	while (!g_bShutdown)
	{
		networkMgr->Update();
		if(SkipForFixedFrame())
			playerMgr->Update();
	}

	networkMgr->Terminate();
	return 0;
}

bool SkipForFixedFrame()
{
	if ((timeGetTime() - g_oldTick) < (1000/FPS))
	{
		return false;
	}

	g_oldTick += (1000 / FPS);
	return true;
}