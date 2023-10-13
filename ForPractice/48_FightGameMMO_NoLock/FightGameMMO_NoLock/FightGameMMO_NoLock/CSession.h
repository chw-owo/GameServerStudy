#pragma once
#include "Config.h"
#include "CPacket.h"
#include "CRingBuffer.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

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

class CSession
{
public:
	void Initialize(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		_socketAlive = 0;

		_ID = ID;
		_sock = sock;
		_addr = addr;
		_IOCount = 0;
		_sendFlag = 0;
		_releaseFlag = 0;

		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

public:
	volatile long _socketAlive = 0;
	__int64 _ID;
	SOCKET _sock;
	SOCKADDR_IN _addr;

	CRingBuffer _recvBuf;
	CRingBuffer _sendBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvOvl;
	NetworkOverlapped _sendOvl;
	NetworkOverlapped _releaseOvl;

	volatile long _IOCount;
	volatile long _sendFlag;
	volatile long _releaseFlag;
};

