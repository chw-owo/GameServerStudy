#include "Main.h"

int main(int argc, char* argv[])
{
	timeBeginPeriod(1);
	NetworkManager* networkMgr = NetworkManager::GetInstance();
	PlayerManager* playerMgr = PlayerManager::GetInstance();

	if (!networkMgr->Initialize()) return 0;

	while (1)
	{
		if (!networkMgr->Update()) break;
		playerMgr->Update();
		WaitForFrame();
	}

	networkMgr->Terminate();
	return 0;
}

void WaitForFrame()
{
	static DWORD oldTick = timeGetTime();
	DWORD deltaTick = timeGetTime() - oldTick;
	oldTick += 50;

	if (deltaTick < 50)
	{
		Sleep(50 - deltaTick);
	}
}