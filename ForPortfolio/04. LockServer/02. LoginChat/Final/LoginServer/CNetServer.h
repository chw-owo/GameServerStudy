#pragma once
#include "Config.h"
#ifdef NETSERVER

#include "CMonitorManager.h"
#include "CNetSession.h"
#include "CNetMsg.h"
#include "CNetJob.h"
#include "CObjectPool.h"

#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

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
	void SleepForFixedSend();

private:
	inline CNetSession* AcquireSessionUsage(unsigned __int64 sessionID);
	inline void ReleaseSessionUsage(CNetSession* pSession);
	inline void IncrementIOCount(CNetSession* pSession);
	inline void DecrementIOCount(CNetSession* pSession);

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
	queue<unsigned __int64> _releaseQ;
	SRWLOCK _releaseLock;

public:
	unordered_map<unsigned __int64, CNetSession*> _sessions;
	CObjectPool<CNetSession>* _pSessionPool;
	volatile __int64 _sessionID = 0;
	SRWLOCK _sessionsLock;

public:
	DWORD _oldTick;

	// For Monitor
public:
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

	inline long GetSessionCount() { return _sessions.size(); }
	inline long GetAcceptTotal() { return _acceptTotal; }
	inline long GetDisconnectTotal() { return _disconnectTotal; }

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