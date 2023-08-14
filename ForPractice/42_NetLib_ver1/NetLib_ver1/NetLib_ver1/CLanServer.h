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
	bool NetworkStart();

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
	int GetAcceptTPS() { return _acceptTPS; }
	int GetRecvMsgTPS() { return _recvMsgTPS; }
	int GetSendMsgTPS() { return _sendMsgTPS; }
	int GetSessionCount() { return _sessionCnt; }

// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	static unsigned int WINAPI ReleaseThread(void* arg);

private:
	void HandleRecvCP(__int64 sessionID, int recvBytes);
	void HandleSendCP(__int64 sessionID, int sendBytes);
	void RecvPost(CSession* pSession);
	void SendPost(CSession* pSession);

private:
	bool _alive = false;
	SOCKET _listenSock;
	int _acceptTPS = 0;
	int _recvMsgTPS = 0;
	int _sendMsgTPS = 0;
	int _sessionCnt = 0;

private:
	HANDLE _acceptThread;
	HANDLE _hReleaseCP;
	HANDLE _releaseThread;
	HANDLE _hNetworkCP;
	HANDLE* _networkThreads;
	int _networkThreadCnt;

};


