#include "GameNetServer.h"
#include "Main.h"

GameNetServer::GameNetServer()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpuCnt = (int)si.dwNumberOfProcessors;

	if (!NetworkStart(dfSERVER_IP, dfNETSERVER_PORT, cpuCnt, false, 100))
		g_bShutdown = true;
}

GameNetServer::~GameNetServer()
{
	NetworkStop();
}

GameNetServer* GameNetServer::GetInstance()
{
	static GameNetServer _GameNetServer;
	return  &_GameNetServer;
}

bool GameNetServer::OnConnectRequest()
{
	// TO-DO: Invalid Check
	return true;
}

void GameNetServer::OnAcceptClient()
{
}

void GameNetServer::OnReleaseClient(__int64 sessionID)
{
}

void GameNetServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	__int64 echo;
	*packet >> echo;

	//::printf("%llu\n", echo);

	CPacket sendPacket(dfNET_HEADER_LEN);;
	sendPacket << echo;
	SendPacket(sessionID, &sendPacket);
}

void GameNetServer::OnSend(__int64 sessionID, int sendSize)
{
}

void GameNetServer::OnError(int errorCode, wchar_t* errorMsg)
{
}
