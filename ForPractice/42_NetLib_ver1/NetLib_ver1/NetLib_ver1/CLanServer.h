#pragma once
#include "CSession.h"
#include "CPacket.h"
#include "Protocol.h"

#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
using namespace std;

class CLanServer
{
// Called in Content
protected:
	bool NetworkStart(const wchar_t* IP, short port, 
		int numOfWorkerThreads, bool nagle, int sessionMax);
public:
	void NetworkStop();

protected:
	bool Disconnect(__int64 sessionID);
	bool SendPacket(__int64 sessionID, CPacket* packet);

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAcceptClient() = 0;
	virtual void OnReleaseClient(__int64 sessionID) = 0;
	virtual void OnRecv(__int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(__int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;

protected:
	inline int GetAcceptTotal() { return _acceptTotal; }
	inline int GetDisconnectTotal() { return _disconnectTotal; }
	inline int GetAcceptTPS() { return _acceptTPS; }
	inline int GetDisconnectTPS() { return _disconnectTPS; }
	inline int GetRecvMsgTPS() { return _recvMsgTPS; }
	inline int GetSendMsgTPS() { return _sendMsgTPS; }
	inline long GetSessionCount() { return _sessionCnt; }

// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	static unsigned int WINAPI ReleaseThread(void* arg);
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	void HandleRecvCP(__int64 sessionID, int recvBytes);
	void HandleSendCP(__int64 sessionID, int sendBytes);
	void RecvPost(CSession* pSession);
	void SendPost(CSession* pSession);

private:
	wchar_t _IP[10]; 
	short _port;
	int _numOfWorkerThreads; 
	bool _nagle; 
	int _sessionMax;

private:
	bool _alive = false;
	volatile long _sessionCnt = 0;
	SOCKET _listenSock;
	unordered_map<__int64, CSession*> _SessionMap;
	SRWLOCK _SessionMapLock;

private:
	int _acceptTotal = 0;
	int _disconnectTotal = 0;

	int _acceptTPS = 0;
	int _disconnectTPS = 0;
	int _recvMsgTPS = 0;
	int _sendMsgTPS = 0;

	int _acceptCnt = 0;
	int _disconnectCnt = 0;
	int _recvMsgCnt = 0;
	int _sendMsgCnt = 0;

private:
	HANDLE _acceptThread;
	HANDLE _releaseThread;
	HANDLE _monitorThread;
	HANDLE* _networkThreads;
	HANDLE _hReleaseCP;
	HANDLE _hNetworkCP;

};


