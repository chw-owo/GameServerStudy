#pragma once
#include "CNetServer.h"
#include "CEchoGroup.h"
#include "CLoginGroup.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"

class CServer : public CNetServer
{
public:
	CServer();
	~CServer();
	bool Initialize();
	void Terminate();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);
	bool OnConnectRequest(WCHAR addr[dfADDRESS_LEN]);
	void OnAcceptClient(unsigned __int64 sessionID, WCHAR addr[dfADDRESS_LEN]);

private:
	void OnReleaseClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CPacket* packet);
	void OnSend(unsigned __int64 sessionID);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

public:
	CEchoGroup* _pEchoGroup;
	CLoginGroup* _pLoginGroup;
	HANDLE _MonitorThread;
	static unsigned int WINAPI MonitorThread(void* arg);
};

