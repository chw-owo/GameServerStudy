#include "CGameServer.h"

bool CGameServer::Initialize()
{
    return false;
}

void CGameServer::Terminate()
{
}

void CGameServer::OnInitialize()
{
}

void CGameServer::OnTerminate()
{
}

void CGameServer::OnThreadTerminate(wchar_t* threadName)
{
}

void CGameServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CGameServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}

bool CGameServer::OnConnectRequest()
{
    return false;
}

void CGameServer::OnAcceptClient(__int64 sessionID)
{
}

void CGameServer::OnReleaseClient(__int64 sessionID)
{
}

void CGameServer::OnRecv(__int64 sessionID, CPacket* packet)
{
}

void CGameServer::OnSend(__int64 sessionID, int sendSize)
{
}
