#include "CNetServer.h"

CNetServer::CNetServer()
{
}

CNetServer::~CNetServer()
{
}

bool CNetServer::Start()
{
	return false;
}

void CNetServer::Stop()
{
}

int CNetServer::GetSessionCount()
{
	return 0;
}

bool CNetServer::Disconnect(__int64 sessionID)
{
	return false;
}

bool CNetServer::SendPacket(__int64 sessionId, CPacket* packet)
{
	return false;
}
