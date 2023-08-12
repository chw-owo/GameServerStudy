#pragma once
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <unordered_map>
#include "RingBuffer.h"
using namespace std;

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

class Session
{
public:
	Session(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
		: _ID(ID), _sock(sock), _addr(addr), _IOCount(0), _sendFlag(0)
	{
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
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
	RingBuffer _sendBuf;
	WSABUF _wsaRecvbuf[2];
	WSABUF _wsaSendbuf[2];
	NetworkOverlapped _recvOvl;
	NetworkOverlapped _sendOvl;

	// For Synchronization
	CRITICAL_SECTION _cs;
	volatile long _IOCount;
	volatile long _sendFlag;
};

extern unordered_map<int, Session*> g_SessionMap;
extern SRWLOCK g_SessionMapLock;