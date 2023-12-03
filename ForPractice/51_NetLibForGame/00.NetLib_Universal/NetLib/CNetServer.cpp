#include "CNetServer.h"

CNetServer::CNetServer()
{
}

bool CNetServer::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle)
{
    return false;
}

bool CNetServer::NetworkTerminate()
{
    return false;
}

bool CNetServer::Disconnect(__int64 sessionID)
{
    return false;
}

bool CNetServer::SendPacket(__int64 sessionID, CPacket* packet)
{
    return false;
}

void CNetServer::OnInitialize()
{
}

void CNetServer::OnTerminate()
{
}

void CNetServer::OnThreadTerminate(wchar_t* threadName)
{
}

bool CNetServer::OnConnectRequest()
{
    return false;
}

void CNetServer::OnAcceptClient(__int64 sessionID)
{
}

void CNetServer::OnReleaseClient(__int64 sessionID)
{
}

void CNetServer::OnRecv(__int64 sessionID, CPacket* packet)
{
}

void CNetServer::OnSend(__int64 sessionID, int sendSize)
{
}

void CNetServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CNetServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}

unsigned int __stdcall CNetServer::AcceptThread(void* arg)
{
    return 0;
}

unsigned int __stdcall CNetServer::NetworkThread(void* arg)
{
    return 0;
}

bool CNetServer::ReleaseSession(__int64 sessionID)
{
    return false;
}

bool CNetServer::HandleRecvCP(CSession* pSession, int recvBytes)
{
    return false;
}

bool CNetServer::HandleSendCP(CSession* pSession, int sendBytes)
{
    return false;
}

bool CNetServer::RecvPost(CSession* pSession)
{
    return false;
}

bool CNetServer::SendPost(CSession* pSession)
{
    return false;
}
