#pragma once
#include "CNetServer.h"

class GameNetServer: public CNetServer
{
private:
	GameNetServer();
	~GameNetServer();

public:
	static GameNetServer* GetInstance();

private:
	static GameNetServer _GameNetServer;

private:
	bool OnConnectRequest();
	void OnAcceptClient();
	void OnReleaseClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);
};

