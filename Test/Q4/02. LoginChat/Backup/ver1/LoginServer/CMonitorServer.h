#pragma once
#include "CChattingServer.h"
#include "CLoginServer.h"

class CMonitorServer
{
public:
	CMonitorServer();
	~CMonitorServer() { Terminate(); }

public:
	bool Initialize(CLoginServer* LoginServer, CChattingServer* ChattingServer);
	void Terminate() { _serverAlive = false; }
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	bool _serverAlive = true;
	HANDLE _monitorThread;

private:
	struct Servers
	{
		CLoginServer* _login;
		CChattingServer* _chatting;
	};
	Servers _servers;
};

