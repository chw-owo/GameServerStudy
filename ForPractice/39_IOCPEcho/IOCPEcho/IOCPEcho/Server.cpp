#include "Server.h"

Server::Server()
{
}

Server* Server::GetInstance()
{
	static Server _server;
	return &_server;
}

void Server::onAccept(__int64 sessionID)
{
}

void Server::onMessage(__int64 sessionID, SerializePacket* packet, int threadID)
{
	__int64 echo;
	*packet >> echo;

	//::printf("%llu\n", echo);

	SerializePacket sendPacket;
	sendPacket << echo;
	SendMsg(sessionID, &sendPacket, threadID);
}

void Server::SendMsg(__int64 sessionID, SerializePacket* packet, int threadID)
{
	NetLib::GetInstance()->MsgToSendData(sessionID, packet, threadID);
}

void Server::onRelease(__int64 sessionID)
{

}


