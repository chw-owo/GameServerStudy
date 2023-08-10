#pragma once
#include "SerializePacket.h"
#include "Protocol.h"
#include "Server.h"
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
	unordered_map<unsigned int, int> ThreadIDMap;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	void HandleRecvCP(__int64 sessionID, int recvBytes, int threadID);
	void HandleSendCP(__int64 sessionID, int sendBytes, int threadID);

private:
	void RecvPost(Session* pSession, int threadID);
	void SendPost(Session* pSession, int threadID);
	void RecvDataToMsg(Session* pSession);
	void ReleaseSession(Session* pSession);

public:
	void MsgToSendData(__int64 sessionID, SerializePacket* packet);
};
