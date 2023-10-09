#pragma once
#include "SerializePacket.h"
#include "Protocol.h"
#include "SessionMap.h"
#include "Session.h"
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
using namespace std;

class NetLib
{
protected:
	NetLib();
	~NetLib();

private:
	static NetLib _netLib;

private:
	SOCKET _listenSock;
	SessionMap* _sessionMap;
	__int64 _sessionIDSupplier = 0;

private:	
	HANDLE _hNetworkCP;
	HANDLE* _networkThreads;
	HANDLE _acceptThread;
	int _networkThreadCnt;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	
private:
	void ReleaseSession(__int64 sessionID);
	void HandleRecvCP(__int64 sessionID, int recvBytes);
	void HandleSendCP(__int64 sessionID, int sendBytes);
	void RecvPost(Session* pSession);
	void SendPost(Session* pSession);

protected:
	void Disconnect(__int64 sessionID);
	void SendPacket(__int64 sessionID, SerializePacket* packet);

protected:	
	virtual void Monitor() = 0;
	virtual void OnAccept(__int64 sessionID) = 0;
	virtual void OnRelease(__int64 sessionID) = 0;
	virtual void OnRecv(__int64 sessionID, SerializePacket* packet) = 0;
	
protected: // For Monitor
	int GetIOPendCnt() { return _IOPendCnt; }
	void ResetIOPendCnt() { _IOPendCnt = 0; }
	int _IOPendCnt = 0;
};
