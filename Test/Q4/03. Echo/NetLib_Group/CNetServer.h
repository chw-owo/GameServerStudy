#pragma once
#include "Config.h"
#ifdef NETSERVER

#include "CLockFreeStack.h"
#include "CSession.h"
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

class CGroup;
class CNetServer
{
	friend CGroup;

protected:
	CNetServer();
	~CNetServer() {};

protected:
	bool NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle);
	bool NetworkTerminate();

protected:
	bool Disconnect(unsigned __int64 sessionID);
	bool SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect = false);
	
protected:
	bool MoveGroup(unsigned __int64 sessionID, CGroup* pGroup);
	bool RegisterGroup(CGroup* pGroup);
	bool RemoveGroup(CGroup* pGroup);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual bool OnConnectRequest(WCHAR addr[dfADDRESS_LEN]) = 0;
	virtual void OnAcceptClient(unsigned __int64 sessionID, WCHAR addr[dfADDRESS_LEN]) = 0;

protected:
	virtual void OnReleaseClient(unsigned __int64 sessionID) = 0;
	virtual void OnRecv(unsigned __int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(unsigned __int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

protected:
	inline void UpdateMonitorData()
	{
		_acceptTPS = InterlockedExchange(&_acceptCnt, 0);
		_disconnectTPS = InterlockedExchange(&_disconnectCnt, 0);
		_recvTPS = InterlockedExchange(&_recvCnt, 0);
		_sendTPS = InterlockedExchange(&_sendCnt, 0);

		_acceptTotal += _acceptTPS;
		_disconnectTotal += _disconnectTPS;
		_recvTotal += _recvTPS;
		_sendTotal += _sendTPS;
	}

	inline long GetSessionCount() { return _sessionCnt; }

	inline long GetAcceptTotal() { return _acceptTotal; }
	inline long GetDisconnectTotal() { return _disconnectTotal; }
	inline long GetRecvTotal() { return _recvTotal; }
	inline long GetSendTotal() { return _sendTotal; }

	inline long GetRecvTPS() { return _recvTPS; }
	inline long GetSendTPS() { return _sendTPS; }
	inline long GetAcceptTPS() { return _acceptTPS; }
	inline long GetDisconnectTPS() { return _disconnectTPS; }

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	void HandleRelease(unsigned __int64 sessionID);
	void HandleRecvCP(CSession* pSession, int recvBytes);
	void HandleSendCP(CSession* pSession, int sendBytes);
	bool RecvPost(CSession* pSession);
	bool SendPost(CSession* pSession);

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

private:
	SOCKET _listenSock;
	volatile long _networkAlive = 0;

private:
	HANDLE _acceptThread;
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;

public:
#define __ID_BIT__ 47
	CSession* _sessions[dfSESSION_MAX] = { nullptr, };
	CLockFreeStack<long> _emptyIdx;
	volatile long _sessionCnt = 0;
	volatile __int64 _sessionID = 0;
	unsigned __int64 _indexMask = 0;
	unsigned __int64 _idMask = 0;

public:
	ValidFlag _releaseFlag;

private:
	long _acceptTotal = 0;
	long _disconnectTotal = 0;
	long _recvTotal = 0;
	long _sendTotal = 0;

	long _acceptTPS = 0;
	long _disconnectTPS = 0;
	long _recvTPS = 0;
	long _sendTPS = 0;

	volatile long _acceptCnt = 0;
	volatile long _disconnectCnt = 0;
	volatile long _recvCnt = 0;
	volatile long _sendCnt = 0;

private:
	unordered_map<CGroup*, HANDLE> _groupThreads;
};
#endif