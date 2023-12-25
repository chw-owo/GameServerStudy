#pragma once
#include "CNetServer.h"
#include "CMonitorData.h"
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
	void OnRecv(unsigned __int64 sessionID, CRecvNetPacket* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);

private:
	void HandleAccept(unsigned __int64 sessionID);
	void HandleRelease(unsigned __int64 sessionID);
	void HandleRecv(unsigned __int64 sessionID, CRecvNetPacket* packet);

private:
	void Handle_REQ_LOGIN(unsigned __int64 sessionID, CRecvNetPacket* packet);
	void SendMonitorPackets(unsigned __int64 sessionID);
	void SetDataToPacket(unsigned __int64 sessionID, BYTE type, BYTE serverNo, CData* data);
	void ReqSendUnicast(unsigned __int64 sessionID, CNetPacket* packet);

private:
	static unsigned int WINAPI JobThread(void* arg);
	static unsigned int WINAPI SendThread(void* arg);
	void SleepForFrame();

private:
	bool _serverAlive = true; 
	long _signal = 0;

private:
	HANDLE _jobThread;
	HANDLE _sendThread;

private:
	vector<unsigned __int64> _sessions;
};

