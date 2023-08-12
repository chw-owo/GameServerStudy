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
	HANDLE _hReleaseCP;
	HANDLE* _networkThreads;
	HANDLE _releaseThread;
	HANDLE _acceptThread;
	int _networkThreadCnt;

	unordered_map<unsigned int, int> ThreadIDMap;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	static unsigned int WINAPI ReleaseThread(void* arg);

private:
	void HandleRecvCP(__int64 sessionID, int recvBytes, int threadID);
	void HandleSendCP(__int64 sessionID, int sendBytes, int threadID);

private:
	void RecvPost(__int64 sessionID, int threadID);
	void SendPost(__int64 sessionID, int threadID);
	void RecvDataToMsg(__int64 sessionID, int threadID);

public:
	void MsgToSendData(__int64 sessionID, SerializePacket* packet, int threadID);
};
