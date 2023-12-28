#pragma once
#include "Config.h"

/*
#include "CLockFreeStack.h"
#include "CSession.h"
#include <ws2tcpip.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")

class CLanClient
{
protected:
	CLanClient() {};
	~CLanClient() {};

protected:
	void Initialize();
	void Terminate();

protected:
	bool Connect(const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle);
	bool Disconnect();
	bool SendPacket(CPacket* packet);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual void OnEnterServer() = 0;
	virtual void OnLeaveServer() = 0;
	virtual void OnRecv(CPacket* packet) = 0;
	virtual void OnSend(int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

protected:
	inline void UpdateMonitorData()
	{
		long recvCnt = InterlockedExchange(&_recvCnt, 0);
		long sendCnt = InterlockedExchange(&_sendCnt, 0);
		_recvTPS = recvCnt;
		_sendTPS = sendCnt;
	}

	inline int GetRecvTPS() { return _recvTPS; }
	inline int GetSendTPS() { return _sendTPS; }

	// Called in Network Library
private:
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	void HandleRelease();
	bool HandleRecvCP(int recvBytes);
	bool HandleSendCP(int sendBytes);
	bool RecvPost();
	bool SendPost();

private:
	wchar_t _IP[10];
	short _port;
	bool _nagle;
	int _numOfThreads;
	int _numOfRunnings;

private:
	CSession* _clientSession;
	volatile long _networkAlive = 0;

private:
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;

public:
	ValidFlag _releaseFlag;
	
private:
	int _recvTPS = 0;
	int _sendTPS = 0;
	volatile long _recvCnt = 0;
	volatile long _sendCnt = 0;
};
*/