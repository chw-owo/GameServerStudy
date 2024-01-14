#pragma once
#include "Config.h"
#include "CLanClient.h"
#include "CLanMsg.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"

class CLoginServer;
class CMonitorClient : public CLanClient
{
public:
	bool Initialize(CLoginServer* pLoginServer);
	void Terminate();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);

private:
	void OnEnterServer();
	void OnLeaveServer();
	void OnRecv(CLanMsg* packet);
	void OnSend(int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	inline void SleepForFixedFrame();
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	inline void SetDataToPacket(BYTE type, int val, int time);
	inline void ReqSendUnicast(CLanSendPacket* packet);

public:
	CLoginServer* _pLoginServer;

private:
	bool _serverAlive = true;
	bool _connected = false;

private:
	HANDLE _monitorThread;
	int _timeGap = 1000;
	DWORD _oldTick;
};

