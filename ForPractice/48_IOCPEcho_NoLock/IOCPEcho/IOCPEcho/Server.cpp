#include "Server.h"

Server::Server()
{
}

Server* Server::GetInstance()
{
	static Server _server;
	return &_server;
}

void Server::Monitor()
{
	::printf("IO Pending: %d\n", GetIOPendCnt());
	ResetIOPendCnt();
}

void Server::OnAccept(__int64 sessionID)
{
}

void Server::OnRecv(__int64 sessionID, SerializePacket* packet)
{
	__int64 echo;
	*packet >> echo;

	// ::printf("%llu\n", echo);

	SerializePacket* sendPacket = new SerializePacket;
	*sendPacket << echo;
	SendPacket(sessionID, sendPacket);
}

void Server::OnRelease(__int64 sessionID)
{

}


