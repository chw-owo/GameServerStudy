#include "CMainServer.h"

bool CMainServer::Initialize()
{
    return false;
}

void CMainServer::Terminate()
{
}

unsigned int __stdcall CMainServer::UpdateThread(void* arg)
{
    CMainServer* pMainServer = (CMainServer*)arg;

    while(1)
    {
        __int64 sessionID = 0;
        bool requested = false;
        if (requested)
        {
            CDungeonServer* pDungeonServer = new CDungeonServer(static_cast<CNetServer*>(pMainServer));
            
            pMainServer->RegisterGroup(static_cast<CGroup*>(pDungeonServer));
            pMainServer->MoveSession(pDungeonServer, sessionID);

            pMainServer->RemoveGroup(static_cast<CGroup*>(pDungeonServer));
        }


    }
    return 0;
}


void CMainServer::OnInitialize()
{
}

void CMainServer::OnTerminate()
{
}

void CMainServer::OnThreadTerminate(wchar_t* threadName)
{
}

void CMainServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CMainServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}

bool CMainServer::OnConnectRequest()
{
    return false;
}

void CMainServer::OnAcceptClient(__int64 sessionID)
{
}

void CMainServer::OnReleaseClient(__int64 sessionID)
{
}

void CMainServer::OnRecv(__int64 sessionID, CPacket* packet)
{
}

void CMainServer::OnSend(__int64 sessionID, int sendSize)
{
}
