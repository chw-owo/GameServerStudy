#pragma once
#include "Config.h"
#include "CLanClient.h"
#include "CLanPacket.h"
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
	void OnRecv(CRecvLanPacket* packet);
	void OnSend(int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	static unsigned int WINAPI PrintThread(void* arg);
	static unsigned int WINAPI SendThread(void* arg);

private:
	void SetDataToPacket(BYTE type, int val, int time);
	void ReqSendUnicast(CLanPacket* packet);

private:
	bool _serverAlive = true;
	CChattingServer* _pChattingServer;

private:
	HANDLE _printThread;
	HANDLE _sendThread;

};

