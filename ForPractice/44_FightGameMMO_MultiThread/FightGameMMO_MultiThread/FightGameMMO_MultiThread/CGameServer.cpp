#include "CGameServer.h"
#include "Main.h"

CGameServer::CGameServer()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpuCnt = (int)si.dwNumberOfProcessors;

	if (!NetworkStart(dfSERVER_IP, dfSERVER_PORT, cpuCnt, false, dfSESSION_MAX))
		g_bShutdown = true;
}

CGameServer::~CGameServer()
{
	Terminate();
}

CGameServer* CGameServer::GetInstance()
{
	static CGameServer _CGameServer;
	return  &_CGameServer;
}

void CGameServer::Update()
{
}

void CGameServer::PrintNetworkData()
{
	UpdateMonitorData();

	printf("Session: %d\n"
		"Accept Total: %d\n"
		"Disconnect Total: %d\n"
		"Accept TPS: %d\n"
		"Disconnect TPS: %d\n"
		"Recv Msg TPS: %d\n"
		"Send Msg TPS: %d\n\n",
		GetSessionCount(),
		GetAcceptTotal(),
		GetDisconnectTotal(),
		GetAcceptTPS(),
		GetDisconnectTPS(),
		GetRecvMsgTPS(),
		GetSendMsgTPS());
}

bool CGameServer::OnConnectRequest()
{
	// TO-DO: Invalid Check
	return true;
}

void CGameServer::OnAcceptClient()
{
}

void CGameServer::OnReleaseClient(__int64 sessionID)
{
}

void CGameServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CGameServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	__int64 echo;
	*packet >> echo;

	//::printf("%llu\n", echo);

	CPacket sendPacket(dfHEADER_LEN);;
	sendPacket << echo;
	SendPacket(sessionID, &sendPacket);
}

void CGameServer::OnSend(__int64 sessionID, int sendSize)
{
}
