#pragma once
#include "CLanClient.h"
#include "CSystemLog.h"

#include "CommonProtocol.h"
#include "CPacket.h"

class CChattingServer;
class CMonitorClient // : public CLanClient
{
public:
	bool Initialize(CChattingServer* pChattingServer);
	void Terminate();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);

private:
	void OnEnterServer();
	void OnLeaveServer();
	void OnRecv(CPacket* packet);
	void OnSend(int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	static unsigned int WINAPI MonitorThread(void* arg);
	HANDLE _monitorThread;
	CChattingServer* _pChattingServer;
};

