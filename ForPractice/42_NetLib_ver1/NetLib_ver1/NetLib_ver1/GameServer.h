#pragma once
#include "CLanServer.h"

class GameServer : public CLanServer
{
private:
	GameServer();
	~GameServer();

public:
	static GameServer* GetInstance();

private:
	static GameServer _gameServer;

private:
	bool OnConnectRequest();
	void OnAcceptClient();
	void OnReleaseClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);
};

