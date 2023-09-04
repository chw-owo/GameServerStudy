#pragma once
#include "CPacket.h"
#include "CRingBuffer.h"
#include <ws2tcpip.h>
#include <synchapi.h>
#pragma comment(lib, "ws2_32.lib")


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

class CSession
{
public:
	CSession()
	{
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		InitializeSRWLock(&_lock);
	}

public:
	void Initialize(__int64 ID, SOCKET sock, SOCKADDR_IN addr);

public:
	bool _alive;
	__int64 _ID;
	SOCKET _sock;
	SOCKADDR_IN _addr;

	CRingBuffer _recvBuf;
	CRingBuffer _sendBuf;
	WSABUF _wsaRecvbuf[2];
	WSABUF _wsaSendbuf[2];
	NetworkOverlapped _recvOvl;
	NetworkOverlapped _sendOvl;

	// For Synchronization
	SRWLOCK _lock;
	volatile long _IOCount;
	volatile long _sendFlag;
};


