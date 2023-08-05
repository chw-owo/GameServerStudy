#pragma once
#include "SerializePacket.h"
#include "Session.h"
#include <ws2tcpip.h>
#include <process.h>

class NetworkManager
{
private:
	NetworkManager();
	~NetworkManager();

public:
	static NetworkManager* GetInstance();

private:
	static NetworkManager _networkManager;

private:
	SOCKET _listenSock;
	__int64 _sessionIDSupplier = 0;

private:
	HANDLE _hEchoCP;
	HANDLE _hNetworkCP;
	int _echoThreadCnt;
	int _networkThreadCnt;
	HANDLE _acceptThread;
	HANDLE* _echoThreads;
	HANDLE* _networkThreads;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	static unsigned int WINAPI EchoThread(void* arg);

private:
	int RecvPost(Session* pSession);
	int SendPost(Session* pSession);
	void ReleaseSession(Session* pSession);

private:
	void EnqueueUnicast(char* msg, int size, Session* pSession);

private:
	void HandleEchoRecv(Session* pSession, int recvBytes);
	void HandleEchoSend(Session* pSession, int sendBytes);

private:
	void GetData_Echo(RingBuffer* buffer, int size, char* data);

private:
	void SetData_Echo(SerializePacket* buffer, int size, char* data);
};