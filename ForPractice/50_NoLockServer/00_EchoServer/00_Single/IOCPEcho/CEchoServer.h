#pragma once
#include "CPacket.h"
#include "CLanServer.h"

class CEchoServer: public CLanServer
{
public:
	CEchoServer();
	~CEchoServer() {}

private:
	bool OnConnectRequest();
	void OnAcceptClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);
	void OnReleaseClient(__int64 sessionID);

private:
	 void OnInitialize();
	 void OnTerminate();
	 void OnThreadTerminate(wchar_t* threadName);
	 void OnError(int errorCode, wchar_t* errorMsg);
};

