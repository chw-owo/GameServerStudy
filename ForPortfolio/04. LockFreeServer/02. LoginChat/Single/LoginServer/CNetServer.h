#pragma once
#include "Config.h"
#ifdef NETSERVER

#include "CLockFreeStack.h"
#include "CNetSession.h"
#include "CNetMsg.h"
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
	bool NetworkInitialize(const wchar_t* IP, short port, long sendTime,
		int numOfThreads, int numOfRunnings, bool nagle, bool monitorServer = false);
	bool NetworkTerminate();

protected:
	bool Disconnect(unsigned __int64 sessionID);
	bool SendPacket(unsigned __int64 sessionID, CNetSendPacket* packet);
	void EnqueueJob(unsigned __int64 sessionID, CNetJob* job);
	CNetJob* DequeueJob(unsigned __int64 sessionID);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual bool OnConnectRequest(WCHAR* addr) = 0;
	virtual void OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr) = 0;
	virtual void OnReleaseClient(unsigned __int64 sessionID) = 0;
	virtual void OnRecv(unsigned __int64 sessionID, CNetMsg* packet) = 0;
	virtual void OnSend(unsigned __int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	static unsigned int WINAPI SendThread(void* arg);
	static unsigned int WINAPI ReleaseThread(void* arg);

private:
	inline bool HandleRecvCP(CNetSession* pSession, int recvBytes);
	inline bool HandleSendCP(CNetSession* pSession, int sendBytes);
	inline bool RecvPost(CNetSession* pSession);
	inline bool SendPost(CNetSession* pSession);
	inline bool SendCheck(CNetSession* pSession);

private:
	inline CNetSession* AcquireSessionUsage(unsigned __int64 sessionID);
	inline void ReleaseSessionUsage(CNetSession* pSession);
	inline void IncrementUseCount(CNetSession* pSession);
	inline void DecrementUseCount(CNetSession* pSession);

private:
	void SleepForFixedSend();

public: // For Profilings
	long _Idx = -1;
	long _NetUpdate[2] = { 0, 0 };
	DWORD _netStart = 0;
	DWORD _netSum = 0;

private:
	wchar_t _IP[10];
	short _port;
	long _sendTime;
	bool _nagle;
	int _numOfThreads;
	int _numOfRunnings;

private:
	SOCKET _listenSock;
	volatile long _networkAlive = 0;

private:
	HANDLE _acceptThread;
	HANDLE _releaseThread;
	HANDLE _sendThread;
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;

private:
	long _releaseSignal = 0;
	CLockFreeQueue<unsigned __int64>* _pReleaseQ;
	ValidFlag _releaseFlag;

public: // TO-DO: private
#define __ID_BIT__ 47
	CNetSession* _sessions[dfSESSION_MAX] = { nullptr, };
	CLockFreeStack<long> _emptyIdx;
	volatile __int64 _sessionID = 0;
	unsigned __int64 _indexMask = 0;
	unsigned __int64 _idMask = 0;

public:
	CLockFreeQueue<CNetJob*>* _pJobQueues[dfJOB_QUEUE_CNT] = { nullptr, };
	CTlsPool<CNetJob>* _pJobPool;

public:
	DWORD _oldTick;

	// For Monitor
public:
	inline void UpdateMonitorData()
	{
		long acceptCnt = 0;
		long disconnectCnt = 0;
		long recvCnt = 0;
		long sendCnt = 0;

		for (int i = 0; i < _tlsMax; i++)
		{
			acceptCnt += InterlockedExchange(&_acceptCnts[i], 0);
			disconnectCnt += InterlockedExchange(&_disconnectCnts[i], 0);
			recvCnt += InterlockedExchange(&_recvCnts[i], 0);
			sendCnt += InterlockedExchange(&_sendCnts[i], 0);
		}

		_acceptTotal += acceptCnt;
		_disconnectTotal += disconnectCnt;

		_acceptTPS = acceptCnt;
		_disconnectTPS = disconnectCnt;
		_recvTPS = recvCnt;
		_sendTPS = sendCnt;
	}

	inline long GetSessionCount() { return _sessionCnt; }
	inline long GetAcceptTotal() { return _acceptTotal; }
	inline long GetDisconnectTotal() { return _disconnectTotal; }

	inline long GetRecvTPS() { return _recvTPS; }
	inline long GetSendTPS() { return _sendTPS; }
	inline long GetAcceptTPS() { return _acceptTPS; }
	inline long GetDisconnectTPS() { return _disconnectTPS; }

public:
	CMonitorManager* _mm = nullptr;

private:
	long _sessionCnt = 0;
	long _acceptTotal = 0;
	long _disconnectTotal = 0;

	long _acceptTPS = 0;
	long _disconnectTPS = 0;
	long _recvTPS = 0;
	long _sendTPS = 0;

#define dfTHREAD_MAX 16
	volatile long _acceptCnts[dfTHREAD_MAX] = { 0, };
	volatile long _disconnectCnts[dfTHREAD_MAX] = { 0, };
	volatile long _recvCnts[dfTHREAD_MAX] = { 0, };
	volatile long _sendCnts[dfTHREAD_MAX] = { 0, };
	volatile long _sessionCnts[dfTHREAD_MAX] = { 0, };

	long _tlsMax = 0;
	long _tlsIdx;
};
#endif