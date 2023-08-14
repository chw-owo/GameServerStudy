#include "Main.h"
bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{
    GameServer* pGameServer = GameServer::GetInstance();

    while (!g_bShutdown)
    {
        Sleep(1000);
        if (GetAsyncKeyState(VK_ESCAPE))
        {
            pGameServer->NetworkStop();
        }
    }

    return 0;
}
