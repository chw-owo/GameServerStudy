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
	__int64 GetID() { return _ID; }

	void Initialize(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		::printf("%d: Session Initialize (%016llx - %016llx)\n", GetCurrentThreadId(), _ID, ID);

		InterlockedExchange16(&_disconnect, 0);
		InterlockedExchange(&_validFlag._flag, 0);
		InterlockedExchange(&_sendFlag, 0);

		_ID = ID;
		_sock = sock;
		_addr = addr;

		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

	void Terminate()
	{
		::printf("%d: Session Terminate (%016llx - %016llx)\n", GetCurrentThreadId(), _ID, (__int64)-1);

		InterlockedExchange16(&_disconnect, 1);
		InterlockedExchange(&_validFlag._flag, 1);

		_ID = -1;
		_sock = INVALID_SOCKET;

		_recvBuf.ClearBuffer();
		while (_sendBuf.GetUseSize() > 0)
		{
			CPacket* packet = _sendBuf.Dequeue();
			CPacket::Free(packet);
		}
		while (_tempBuf.GetUseSize() > 0)
		{
			CPacket* packet = _tempBuf.Dequeue();
			CPacket::Free(packet);
		}
	}

private:
	__int64 _ID = -1;	

public:
	volatile short _disconnect = 1;
	volatile long _sendFlag;
	volatile long _sendCount;

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



public:
	typedef union ValidFlag
	{
		struct
		{
			short _releaseFlag;
			short _useCount;
		};
		long _flag;
	};
	volatile ValidFlag _validFlag;
};

