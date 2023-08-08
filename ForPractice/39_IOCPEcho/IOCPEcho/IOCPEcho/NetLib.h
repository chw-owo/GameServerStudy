#pragma once
#include "SerializePacket.h"
#include "Protocol.h"
#include "Server.h"
#include "Session.h"
#include <ws2tcpip.h>
#include <process.h>

class NetLib
{
protected:
	NetLib();
	~NetLib();

public:
	static NetLib* GetInstance();

private:
	static NetLib _netLib;

private:
	SOCKET _listenSock;
	__int64 _sessionIDSupplier = 0;

private:
	
	HANDLE _hNetworkCP;
	int _networkThreadCnt;
	HANDLE* _networkThreads;
	HANDLE _acceptThread;

	//HANDLE _hEchoCP;
	//int _echoThreadCnt;
	//HANDLE* _echoThreads;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	//static unsigned int WINAPI EchoThread(void* arg);

private:
	void HandleRecvCP(__int64 sessionID, int recvBytes);
	void HandleSendCP(__int64 sessionID, int sendBytes);

private:
	void RecvPost(Session* pSession);
	void SendPost(Session* pSession);
	void RecvDataToMsg(Session* pSession);
	void ReleaseSession(Session* pSession);

public:
	void MsgToSendData(__int64 sessionID, SerializePacket* packet);
	
};