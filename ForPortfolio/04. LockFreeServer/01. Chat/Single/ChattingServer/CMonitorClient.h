#pragma once
#include "Config.h"
#include "CLanClient.h"
#include "CLanSendPacket.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"

class CChattingServer;
class CMonitorClient : public CLanClient
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

private:
	bool _serverAlive = true;
	bool _connected = false;
	CChattingServer* _pChattingServer;

private:
	HANDLE _monitorThread;
	int _timeGap = 1000;
	DWORD _oldTick;
};

