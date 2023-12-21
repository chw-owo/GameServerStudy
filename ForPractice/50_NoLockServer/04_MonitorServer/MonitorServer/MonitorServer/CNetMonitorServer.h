#pragma once
#include "CNetServer.h"

#include <vector>
using namespace std;

class CNetMonitorServer : public CNetServer
{
public:
	CNetMonitorServer() {};
	~CNetMonitorServer() { Terminate(); };

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
	void OnAcceptClient(unsigned __int64 sessionID);
	void OnReleaseClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CRecvPacket* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);

private:
	void HandleAccept(unsigned __int64 sessionID);
	void HandleRelease(unsigned __int64 sessionID);
	void HandleRecv(unsigned __int64 sessionID, CRecvPacket* packet);

private:
	void Handle_REQ_LOGIN(unsigned __int64 sessionID, CRecvPacket* packet);

private:
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	bool _serverAlive = true; 
	long _signal = 0;
	HANDLE _monitorThread;

private:
	vector<unsigned __int64> _sessions;
};

