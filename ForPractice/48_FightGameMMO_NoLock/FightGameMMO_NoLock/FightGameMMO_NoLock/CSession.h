#pragma once
#include "Config.h"
#include "CPacket.h"
#include "CRingBuffer.h"
#include "CLockFreeQueue.h"

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
	CSession()
	{
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
	}

public:
	void Initialize(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		_ID = ID;
		_sock = sock;
		_addr = addr;
		_sendFlag = 0;
		_validFlag._flag = 0;

		_recvBuf.ClearBuffer();
		while (_sendBuf.GetUseSize() > 0)
			_sendBuf.Dequeue();
		while (_tempBuf.GetUseSize() > 0)
			_tempBuf.Dequeue();

		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

	void Terminate()
	{
		_ID = -1;
		_sock = INVALID_SOCKET;
		_sendFlag = 0;
		_validFlag._IOCount = 0;
		_validFlag._releaseFlag = 1;
	}

public:
	__int64 _ID;
	SOCKET _sock;
	SOCKADDR_IN _addr;

	CRingBuffer _recvBuf;
	CLockFreeQueue<CPacket*> _sendBuf;
	CLockFreeQueue<CPacket*> _tempBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvOvl;
	NetworkOverlapped _sendOvl;
	NetworkOverlapped _releaseOvl;
	volatile long _sendFlag;
	volatile long _sendCount;

public:
	typedef union ValidFlag
	{
		struct
		{
			short _IOCount;
			short _releaseFlag;
		};

		long _flag;
	};

	volatile ValidFlag _validFlag;
};

