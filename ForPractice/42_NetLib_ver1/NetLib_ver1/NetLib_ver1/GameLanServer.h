#pragma once
#include "CLanServer.h"

class GameLanServer : public CLanServer
{
private:
	GameLanServer();
	~GameLanServer();

public:
	static GameLanServer* GetInstance();

private:
	static GameLanServer _GameLanServer;

private:
	bool OnConnectRequest();
	void OnAcceptClient();
	void OnReleaseClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);


public:
	void PrintMonitorData()
	{
		printf("\nSession: %d\n"
			"Accept TPS: %d\n"
			"Recv Msg TPS: %d\n"
			"Send Msg TPS: %d\n\n",
			GetSessionCount(), GetAcceptTPS(),
			GetRecvMsgTPS(), GetSendMsgTPS());
	}
};

