#include "Main.h"
bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{
    GameLanServer* pGameLanServer = GameLanServer::GetInstance();

    while (!g_bShutdown)
    {
        Sleep(1000);
        if (GetAsyncKeyState(VK_ESCAPE))
        {
            pGameLanServer->NetworkStop();
        }
    }

    return 0;
}
