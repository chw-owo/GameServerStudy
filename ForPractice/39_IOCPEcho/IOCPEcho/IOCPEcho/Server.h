#pragma once
#include "SerializePacket.h"
#include "Session.h"
#include "NetLib.h"

class Server
{
private:
	Server();
	~Server() {}

public:
	static Server* GetInstance();

private:
	static Server _server;

public:
	void onAccept(__int64 sessionID);
	void onMessage(__int64 sessionID, SerializePacket* packet, int threadID);
	void onRelease(__int64 sessionID);

public:
	void SendMsg(__int64 sessionID, SerializePacket* packet, int threadID);
};

