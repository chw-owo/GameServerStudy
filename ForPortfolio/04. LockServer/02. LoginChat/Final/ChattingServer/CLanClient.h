#pragma once
#include "Config.h"
#ifdef LANSERVER

#include "CClientSession.h"
#include "CMonitorManager.h"

#include <ws2tcpip.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

class CLanClient
{
protected:
	CLanClient() {};
	~CLanClient() {};

protected:
	bool NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle, bool monitor);
	void NetworkTerminate();

protected:
	bool Disconnect();
	bool SendPacket(CLanSendPacket* packet);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual void OnEnterServer() = 0;
	virtual void OnLeaveServer() = 0;
	virtual void OnRecv(CLanMsg* packet) = 0;
	virtual void OnSend(int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in Network Library
private:
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	inline void HandleRelease();
	inline bool HandleRecvCP(int recvBytes);
	inline bool HandleSendCP(int sendBytes);
	inline bool RecvPost();
	inline bool SendPost();
	inline bool SendCheck();

private:
	inline void IncrementUseCount();
	inline void DecrementUseCount();

private:
	wchar_t _IP[10];
	short _port;
	bool _nagle;
	int _numOfThreads;
	int _numOfRunnings;

private:
	CClientSession* _client;
	volatile long _networkAlive = 0;
	CMonitorManager* _mm = nullptr;

private:
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;
};
#endif