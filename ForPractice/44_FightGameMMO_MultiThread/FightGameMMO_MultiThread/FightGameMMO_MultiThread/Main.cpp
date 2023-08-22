#include "Main.h"
#include "CGameServer.h"
bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{
    CGameServer* pGameServer = CGameServer::GetInstance();
    while (!g_bShutdown)
    {
        if (GetAsyncKeyState(VK_ESCAPE))
            pGameServer->Terminate();

        pGameServer->Update();
    }

    return 0;
}