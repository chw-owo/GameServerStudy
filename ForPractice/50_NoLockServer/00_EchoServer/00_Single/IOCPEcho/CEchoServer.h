#pragma once
#include "CPacket.h"
#include "CLanServer.h"
#include "Protocol.h"

class CEchoServer: public CLanServer
{
public:
	CEchoServer();
	~CEchoServer() {}

public:
	void Initialize();
	void Terminate();

private:
	bool OnConnectRequest();
	void OnAcceptClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CPacket* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);
	void OnReleaseClient(unsigned __int64 sessionID);

private:
	 void OnInitialize();
	 void OnTerminate();
	 void OnThreadTerminate(wchar_t* threadName);
	 void OnError(int errorCode, wchar_t* errorMsg);
	 void OnDebug(int debugCode, wchar_t* debugMsg);
};

