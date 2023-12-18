#include "CMonitorServer.h"

bool CMonitorServer::Initialize()
{
	return true;
}

void CMonitorServer::Terminate()
{
}

void CMonitorServer::OnInitialize()
{
}

void CMonitorServer::OnTerminate()
{
}

void CMonitorServer::OnThreadTerminate(wchar_t* threadName)
{
}

void CMonitorServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CMonitorServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}

bool CMonitorServer::OnConnectRequest()
{
	return false;
}

void CMonitorServer::OnAcceptClient(unsigned __int64 sessionID)
{
}

void CMonitorServer::OnReleaseClient(unsigned __int64 sessionID)
{
}

void CMonitorServer::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
}

void CMonitorServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}
