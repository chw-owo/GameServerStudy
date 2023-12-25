#pragma once
#include "Config.h"
#ifdef LANSERVER

#include "CLockFreeStack.h"
#include "CLanSession.h"
#include "CRecvLanPacket.h"
#include "CMonitorManager.h"
#include "CLanJob.h"
#include <ws2tcpip.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")

class CLanServer
{
protected:
	CLanServer();
	~CLanServer() {};

protected:
	bool LanworkInitialize(
		const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle, bool monitorServer = false);
	bool LanworkTerminate();

protected:
	bool Disconnect(unsigned __int64 sessionID);
	bool SendPacket(unsigned __int64 sessionID, CLanPacket* packet);
	void EnqueueJob(unsigned __int64 sessionID, CLanJob* job);
	CLanJob* DequeueJob(unsigned __int64 sessionID);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAcceptClient(unsigned __int64 sessionID) = 0;
	virtual void OnReleaseClient(unsigned __int64 sessionID) = 0;
	virtual void OnRecv(unsigned __int64 sessionID, CRecvLanPacket* packet) = 0;
	virtual void OnSend(unsigned __int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in Lanwork Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI LanworkThread(void* arg);

private:
	void HandleRelease(unsigned __int64 sessionID);
	bool HandleRecvCP(CLanSession* pSession, int recvBytes);
	bool HandleSendCP(CLanSession* pSession, int sendBytes);
	bool RecvPost(CLanSession* pSession);
	bool SendPost(CLanSession* pSession);
	bool SendCheck(CLanSession* pSession);

private:
	CLanSession* AcquireSessionUsage(unsigned __int64 sessionID);
	void ReleaseSessionUsage(CLanSession* pSession);
	void IncrementUseCount(CLanSession* pSession);
	void DecrementUseCount(CLanSession* pSession);

private:
	wchar_t _IP[10];
	short _port;
	bool _nagle;
	int _numOfThreads;
	int _numOfRunnings;

private:
	SOCKET _listenSock;
	volatile long _networkAlive = 0;

private:
	HANDLE _acceptThread;
	HANDLE* _networkThreads;
	HANDLE _hLanworkCP;

public: // TO-DO: private
#define __ID_BIT__ 47
	CLanSession* _sessions[dfSESSION_MAX] = { nullptr, };
	CLockFreeStack<long> _emptyIdx;
	volatile long _sessionCnt = 0;
	volatile __int64 _sessionID = 0;
	unsigned __int64 _indexMask = 0;
	unsigned __int64 _idMask = 0;

public:
	CLockFreeQueue<CLanJob*>* _pJobQueues[dfJOB_QUEUE_CNT] = { nullptr, };
	CTlsPool<CLanJob>* _pJobPool;

public:
	ValidFlag _releaseFlag;

	// For Monitor
protected:
	inline void UpdateMonitorData()
	{
		long acceptCnt = InterlockedExchange(&_acceptCnt, 0);
		long disconnectCnt = InterlockedExchange(&_disconnectCnt, 0);
		long recvCnt = InterlockedExchange(&_recvCnt, 0);
		long sendCnt = InterlockedExchange(&_sendCnt, 0);

		_acceptTotal += acceptCnt;
		_disconnectTotal += disconnectCnt;

		_acceptTPS = acceptCnt;
		_disconnectTPS = disconnectCnt;
		_recvTPS = recvCnt;
		_sendTPS = sendCnt;
	}

	inline long GetAcceptTotal() { return _acceptTotal; }
	inline long GetDisconnectTotal() { return _disconnectTotal; }
	inline long GetSessionCount() { return _sessionCnt; }

	inline long GetRecvTPS() { return _recvTPS; }
	inline long GetSendTPS() { return _sendTPS; }
	inline long GetAcceptTPS() { return _acceptTPS; }
	inline long GetDisconnectTPS() { return _disconnectTPS; }

public:
	CMonitorManager* _mm = nullptr;

private:
	long _acceptTotal = 0;
	long _disconnectTotal = 0;

	long _acceptTPS = 0;
	long _disconnectTPS = 0;
	long _recvTPS = 0;
	long _sendTPS = 0;

	volatile long _acceptCnt = 0;
	volatile long _disconnectCnt = 0;
	volatile long _recvCnt = 0;
	volatile long _sendCnt = 0;
};
#endif