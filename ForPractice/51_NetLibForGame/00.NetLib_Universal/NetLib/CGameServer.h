#pragma once
#include "CNetServer.h"

class CGameServer: public CNetServer
{
public:
	CGameServer() {}
	~CGameServer() { Terminate(); };

public:
	bool Initialize();
	void Terminate();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	bool OnConnectRequest();
	void OnAcceptClient(__int64 sessionID);
	void OnReleaseClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);
};

