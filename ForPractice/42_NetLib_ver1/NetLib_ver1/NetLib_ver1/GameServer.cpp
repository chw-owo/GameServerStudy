#include "GameServer.h"
#include "Main.h"

GameServer::GameServer()
{
	if (!NetworkStart())
		g_bShutdown = true;
}

GameServer::~GameServer()
{
	NetworkStop();
}

GameServer* GameServer::GetInstance()
{
    static GameServer _gameServer;
    return  &_gameServer;
}

bool GameServer::OnConnectRequest()
{
    // TO-DO: Invalid Check
    return true;
}

void GameServer::OnAcceptClient()
{
}

void GameServer::OnReleaseClient(__int64 sessionID)
{
}

void GameServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void GameServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	__int64 echo;
	*packet >> echo;

	//::printf("%llu\n", echo);

	CPacket sendPacket;
	sendPacket << echo;
	SendPacket(sessionID, &sendPacket);
}

void GameServer::OnSend(__int64 sessionID, int sendSize)
{
}
