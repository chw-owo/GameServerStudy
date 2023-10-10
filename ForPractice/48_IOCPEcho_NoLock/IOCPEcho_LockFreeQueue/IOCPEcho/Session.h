#pragma once
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <unordered_map>
#include "RingBuffer.h"
#include "LockFreeQueue.h"
using namespace std;

#define WSA_RCVBUF_MAX 2
#define WSA_SNDBUF_MAX 100

enum class NET_TYPE
{
	SEND = 0,
	RECV,
	RELEASE
};

struct NetworkOverlapped
{
	OVERLAPPED _ovl;
	NET_TYPE _type;
};

class Session
{
public:
	Session(int idx) : _idx(idx)
	{
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
	}

	void Initialize(__int64 sessionID, SOCKET sock, SOCKADDR_IN addr) 
	{
		_sessionID = sessionID;
		_sock = sock; 
		_addr = addr; 
		_IOCount = 0; 
		_sendFlag = 0;
		_sendCnt = 0;

		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

	void Clear()
	{
		_sessionID = -1;
		closesocket(_sock);
		_sock = INVALID_SOCKET;

		_recvBuf.ClearBuffer();
		while(_sendBuf.GetUseSize() > 0)
			_sendBuf.Dequeue();
		while (_tempBuf.GetUseSize() > 0)
			_tempBuf.Dequeue();
	}

public:
	int _idx;
	__int64 _sessionID;
	SOCKET _sock;
	SOCKADDR_IN _addr;

	RingBuffer _recvBuf;
	LockFreeQueue_Packet _sendBuf;
	LockFreeQueue_Packet _tempBuf;
	WSABUF _wsaRecvbuf[WSA_RCVBUF_MAX];
	WSABUF _wsaSendbuf[WSA_SNDBUF_MAX];
	int _sendCnt;

	NetworkOverlapped _recvOvl;
	NetworkOverlapped _sendOvl;
	NetworkOverlapped _releaseOvl;

	volatile long _IOCount;
	volatile long _sendFlag;
};