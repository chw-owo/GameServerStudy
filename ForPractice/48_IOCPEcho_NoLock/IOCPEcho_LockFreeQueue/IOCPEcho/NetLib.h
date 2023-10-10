#pragma once
#include "SerializePacket.h"
#include "LockFreePool.h"
#include "LockFreeQueue.h"
#include "Protocol.h"
#include "Session.h"
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
using namespace std;
#define dfSESSION_MAX 150

class NetLib
{
protected:
	NetLib();
	~NetLib();

private:
	static NetLib _netLib;

private:
	SOCKET _listenSock;
	__int64 _sessionIDSupplier = 0;

private:
	Session* _sessions[dfSESSION_MAX] = { nullptr, };
	LockFreeQueue_int _unuseIdxs;

private:	
	HANDLE _hNetworkCP;
	HANDLE* _networkThreads;
	HANDLE _acceptThread;
	int _networkThreadCnt;

protected:
#define PACKET_NUM 4096
	LockFreePool<SerializePacket>* _packetPool;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	
private:
	void ReleaseSession(Session* pSession);
	void HandleRecvCP(Session* pSession, int recvBytes);
	void HandleSendCP(Session* pSession, int sendBytes);
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

};
