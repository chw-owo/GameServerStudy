#pragma once
#include "SerializePacket.h"
#include "NetLib.h"

class Server: public NetLib
{
private:
	Server();
	~Server() {}

public:
	static Server* GetInstance();

private:
	static Server _server;

public:
	void Monitor();

public:
	void OnAccept(__int64 sessionID);
	void OnRecv(__int64 sessionID, SerializePacket* packet);
	void OnRelease(__int64 sessionID);
};

