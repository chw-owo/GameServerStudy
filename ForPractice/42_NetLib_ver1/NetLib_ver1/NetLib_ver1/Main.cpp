#include "Main.h"
bool g_bShutdown = false;

int wmain(int argc, wchar_t* argv[])
{
    CProcessCpuUsage ProcessCPUTime;
    CProcessorCpuUsage ProcessorCPUTime;
    GameLanServer* pGameLanServer = GameLanServer::GetInstance();

    while (!g_bShutdown)
    {
        Sleep(1000);
       
        if (GetAsyncKeyState(VK_SPACE))
        {
            pGameLanServer->NetworkStop();
        }

        pGameLanServer->PrintNetworkData();
        ProcessorCPUTime.PrintCpuData();
        ProcessCPUTime.PrintCpuData();

        printf("=======================================\n\n");
    }

    return 0;
}
