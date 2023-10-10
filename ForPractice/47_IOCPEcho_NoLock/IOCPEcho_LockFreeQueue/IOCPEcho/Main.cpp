#include "Main.h"

bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{
	Server* pServer = Server::GetInstance();

	while (!g_bShutdown)
	{
		Sleep(1000);
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_bShutdown = true;
		}
		// pServer->Monitor();
	}

	return 0;
}