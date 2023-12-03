#include "CDungeonServer.h"

void CDungeonServer::OnInitialize()
{
}

void CDungeonServer::OnTerminate()
{
}

unsigned int __stdcall CDungeonServer::UpdateThread(void* arg)
{
    CDungeonServer* pServer = (CDungeonServer*)arg;
    while(1)
    {
        if (pServer->GetAlive()) break;
        AcquireSRWLockShared(&pServer->_lock);
        ReleaseSRWLockShared(&pServer->_lock);
    }
    return 0;
}

void CDungeonServer::OnRegisterClient(CSession* pSession)
{
}

void CDungeonServer::OnReleaseClient(CSession* pSession)
{
}

void CDungeonServer::OnRecv(CSession* pSession, CPacket* packet)
{
}

void CDungeonServer::OnSend(CSession* pSession, int sendSize)
{
}
