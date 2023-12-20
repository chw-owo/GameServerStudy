#pragma once
#include "Config.h"
#ifdef NETSERVER

#include "CLockFreeStack.h"
#include "CSession.h"
#include "CRecvPacket.h"
#include "CMonitorManager.h"
#include "CJob.h"
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
	bool SendPacket(unsigned __int64 sessionID, CPacket* packet);
	void EnqueueJob(unsigned __int64 sessionID, CJob* job);
	CJob* DequeueJob(unsigned __int64 sessionID);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAcceptClient(unsigned __int64 sessionID) = 0;
	virtual void OnReleaseClient(unsigned __int64 sessionID) = 0;
	virtual void OnRecv(unsigned __int64 sessionID, CRecvPacket* packet) = 0;
	virtual void OnSend(unsigned __int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	void HandleRelease(unsigned __int64 sessionID);
	bool HandleRecvCP(CSession* pSession, int recvBytes);
	bool HandleSendCP(CSession* pSession, int sendBytes);
	bool RecvPost(CSession* pSession);
	bool SendPost(CSession* pSession);
	bool SendCheck(CSession* pSession);

private:
	CSession* AcquireSessionUsage(unsigned __int64 sessionID);
	void ReleaseSessionUsage(CSession* pSession);
	void IncrementUseCount(CSession* pSession);
	void DecrementUseCount(CSession* pSession);

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
	CSession* _sessions[dfSESSION_MAX] = { nullptr, };
	CLockFreeStack<long> _emptyIdx;
	volatile long _sessionCnt = 0;
	volatile __int64 _sessionID = 0;
	unsigned __int64 _indexMask = 0;
	unsigned __int64 _idMask = 0;

public:
	CLockFreeQueue<CJob*>* _pJobQueues[dfSESSION_MAX] = { nullptr, };
	CTlsPool<CJob>* _pJobPool;

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

	long GetProcessCPU() { return _mm->GetProcessCPU(); }
	long GetProcessOnOff() { return _mm->GetProcessOnOff(); }
	long GetProcessMemory() { return _mm->GetProcessMemory(); }

	long GetCPUTotal() { return _mm->GetCPUTotal(); }
	long GetNonpagedTotal() { return _mm->GetNonpagedTotal(); }
	long GetUsableMemoryTotal() { return _mm->GetUsableMemoryTotal(); }
	long GetRecvTotal() { return _mm->GetRecvTotal(); }
	long GetSendTotal() { return _mm->GetSendTotal(); }

private:
	CMonitorManager* _mm = nullptr;

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