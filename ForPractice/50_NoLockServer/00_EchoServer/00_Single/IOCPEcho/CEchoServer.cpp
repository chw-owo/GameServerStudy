#include "CEchoServer.h"

CEchoServer::CEchoServer()
{

}

bool CEchoServer::OnConnectRequest()
{
	return true;
}

void CEchoServer::OnAcceptClient(__int64 sessionID)
{
}

void CEchoServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	__int64 echo;
	*packet >> echo;

	// ::printf("%llu\n", echo);

	CPacket* sendPacket = CPacket::Alloc();
	*sendPacket << echo;
	SendPacket(sessionID, sendPacket);
}

void CEchoServer::OnSend(__int64 sessionID, int sendSize)
{

}

void CEchoServer::OnReleaseClient(__int64 sessionID)
{

}

void CEchoServer::OnInitialize()
{
}

void CEchoServer::OnTerminate()
{
}

void CEchoServer::OnThreadTerminate(wchar_t* threadName)
{
}

void CEchoServer::OnError(int errorCode, wchar_t* errorMsg)
{
}




