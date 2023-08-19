#include "GameLanServer.h"
#include "Main.h"
#define dfSESSION_MAX 10000
GameLanServer::GameLanServer()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpuCnt = (int)si.dwNumberOfProcessors;

	if (!NetworkStart(dfSERVER_IP, dfLANSERVER_PORT, cpuCnt, false, dfSESSION_MAX))
		g_bShutdown = true;
}

GameLanServer::~GameLanServer()
{
	NetworkStop();
}

GameLanServer* GameLanServer::GetInstance()
{
    static GameLanServer _GameLanServer;
    return  &_GameLanServer;
}

bool GameLanServer::OnConnectRequest()
{
    // TO-DO: Invalid Check
    return true;
}

void GameLanServer::OnAcceptClient()
{
}

void GameLanServer::OnReleaseClient(__int64 sessionID)
{
}

void GameLanServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void GameLanServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	__int64 echo;
	*packet >> echo;

	//::printf("%llu\n", echo);

	CPacket sendPacket(dfLAN_HEADER_LEN);;
	sendPacket << echo;
	SendPacket(sessionID, &sendPacket);
}

void GameLanServer::OnSend(__int64 sessionID, int sendSize)
{
}
