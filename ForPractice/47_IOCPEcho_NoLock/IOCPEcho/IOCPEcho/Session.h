#pragma once
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <unordered_map>
#include "RingBuffer.h"
#include "PacketBuffer.h"
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
	Session(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
		: _ID(ID), _sock(sock), _addr(addr), _IOCount(0), _sendFlag(0)
	{
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
		InitializeCriticalSection(&_cs);
	}

	~Session()
	{
		DeleteCriticalSection(&_cs);
	}

public:
	__int64 _ID;
	SOCKET _sock;
	SOCKADDR_IN _addr;

	RingBuffer _recvBuf;
	PacketBuffer _sendBuf;
	WSABUF _wsaRecvbuf[WSA_RCVBUF_MAX];
	WSABUF _wsaSendbuf[WSA_SNDBUF_MAX];
	int _sendCnt;

	NetworkOverlapped _recvOvl;
	NetworkOverlapped _sendOvl;
	NetworkOverlapped _releaseOvl;

	// For Synchronization
	CRITICAL_SECTION _cs;
	volatile long _IOCount;
	volatile long _sendFlag;
};