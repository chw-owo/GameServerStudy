#pragma once
#include "CLanServer.h"
#include "CMonitorData.h"
#include <unordered_map>
#include <synchapi.h>

#pragma comment(lib, "Synchronization.lib")
using namespace std;

class CLanMonitorServer : public CLanServer
{
public:
	CLanMonitorServer() {};
	~CLanMonitorServer() { Terminate(); };

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
	void OnRecv(unsigned __int64 sessionID, CRecvLanPacket* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);

private:
	void HandleAccept(unsigned __int64 sessionID);
	void HandleRelease(unsigned __int64 sessionID);
	void HandleRecv(unsigned __int64 sessionID, CRecvLanPacket* packet);

private:
	void Handle_LOGIN(unsigned __int64 sessionID, CRecvLanPacket* packet);
	void Handle_DATA_UPDATE(unsigned __int64 sessionID, CRecvLanPacket* packet);
	void GetDataFromPacket(CRecvLanPacket* packet, CData* data);

private:
	static unsigned int WINAPI MonitorThread(void* arg);
	static unsigned int WINAPI SaveThread(void* arg);

private:
	bool _serverAlive = true;
	long _signal = 0;

private:
	HANDLE _monitorThread;
	HANDLE _saveThread;

private:
	unordered_map<unsigned __int64, int> _serverID; // Now Not Used
};

