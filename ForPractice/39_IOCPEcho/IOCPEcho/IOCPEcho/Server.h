#pragma once
#pragma comment(lib, "ws2_32")
#include "SerializePacket.h"
#include "RingBuffer.h"
#include <unordered_map>
#include <ws2tcpip.h>
#include <process.h>

using namespace std;

class Server
{
private:
	Server();
	~Server();

public:
	static Server* GetInstance();
	void Monitor();

private:
	static Server _server;

private:
	HANDLE _hCP;
	SOCKET _listenSock;
	HANDLE _acceptThread;
	int _workerThreadCnt;
	HANDLE* _workerThreads;
	CRITICAL_SECTION _cs;

private:
	enum class NET_TYPE
	{
		SEND = 0,
		RECV
	};

	struct NetworkOverlapped
	{
		OVERLAPPED _ovl;
		NET_TYPE _type;
	};

private:
	struct Session
	{
		__int64 _ID;
		volatile long _IOCount = 0;
		SOCKET _sock;
		SOCKADDR_IN _addr;

		WSABUF _wsaRecvbuf;
		WSABUF _wsaSendbuf;
		RingBuffer _recvBuf;
		RingBuffer _sendBuf;
		NetworkOverlapped* _pRecvOvl;
		NetworkOverlapped* _pSendOvl;
	};

private:
	__int64 _sessionIDSupplier = 0;
	unordered_map<int, Session*> _SessionMap;

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI WorkerThread(void* arg);

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