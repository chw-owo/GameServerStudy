#pragma once
#include "Config.h"
#ifdef NETSERVER

#include "CLockFreeStack.h"
#include "CNetSession.h"
#include "CRecvNetPacket.h"
#include "CMonitorManager.h"
#include "CNetJob.h"
#include <ws2tcpip.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")

class CNetServer
{
protected:
	CNetServer();
	~CNetServer() {};

protected:
	bool NetworkInitialize(
		const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle, bool monitorServer = false);
	bool NetworkTerminate();

protected:
	bool Disconnect(unsigned __int64 sessionID);
	bool SendPacket(unsigned __int64 sessionID, CNetPacket* packet);
	void EnqueueJob(unsigned __int64 sessionID, CNetJob* job);
	CNetJob* DequeueJob(unsigned __int64 sessionID);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAcceptClient(unsigned __int64 sessionID) = 0;
	virtual void OnReleaseClient(unsigned __int64 sessionID) = 0;
	virtual void OnRecv(unsigned __int64 sessionID, CRecvNetPacket* packet) = 0;
	virtual void OnSend(unsigned __int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	void HandleRelease(unsigned __int64 sessionID);
	bool HandleRecvCP(CNetSession* pSession, int recvBytes);
	bool HandleSendCP(CNetSession* pSession, int sendBytes);
	bool RecvPost(CNetSession* pSession);
	bool SendPost(CNetSession* pSession);
	bool SendCheck(CNetSession* pSession);

private:
	CNetSession* AcquireSessionUsage(unsigned __int64 sessionID);
	void ReleaseSessionUsage(CNetSession* pSession);
	void IncrementUseCount(CNetSession* pSession);
	void DecrementUseCount(CNetSession* pSession);

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
	HANDLE _hNetworkCP;

public: // TO-DO: private
#define __ID_BIT__ 47
	CNetSession* _sessions[dfSESSION_MAX] = { nullptr, };
	CLockFreeStack<long> _emptyIdx;
	volatile long _sessionCnt = 0;
	volatile __int64 _sessionID = 0;
	unsigned __int64 _indexMask = 0;
	unsigned __int64 _idMask = 0;

public:
	CLockFreeQueue<CNetJob*>* _pJobQueues[dfJOB_QUEUE_CNT] = { nullptr, };
	CTlsPool<CNetJob>* _pJobPool;

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