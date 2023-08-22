#pragma once
#include "CLanServer.h"

class CGameServer : public CLanServer
{
private:
	CGameServer();
	~CGameServer();

public:
	static CGameServer* GetInstance();
	void Update();

private:
	static CGameServer _CGameServer;

public:
	void PrintNetworkData();

private:
	bool OnConnectRequest();
	void OnAcceptClient();
	void OnReleaseClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);

private:
	int _acceptTotal = 0;
};

