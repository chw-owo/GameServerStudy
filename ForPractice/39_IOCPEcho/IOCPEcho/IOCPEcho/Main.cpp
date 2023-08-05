#include "Main.h"

bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{
	NetworkManager* networkManager = NetworkManager::GetInstance();
	ContentManager* contentManager = ContentManager::GetInstance();

	while (!g_bShutdown)
	{
		Sleep(1000);
		if (GetAsyncKeyState(VK_RETURN))
		{
			g_bShutdown = true;
		}
	}

	return 0;
}