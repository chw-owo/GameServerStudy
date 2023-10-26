#include "CEchoServer.h"

CEchoServer g_Server;
int wmain(int argc, wchar_t* argv[])
{
	for(;;)
	{
		Sleep(1000);
		if (GetAsyncKeyState(VK_ESCAPE)) break;
	}

	return 0;
}