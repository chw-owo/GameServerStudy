
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

union ValidFlag
{
	struct
	{
		volatile short _releaseFlag;
		volatile short _useCount;
	};
	volatile long _flag;
};

class CSession
{
public:
	CSession()
	{
		_validFlag._flag = 0;
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
	}

public:
	unsigned __int64 GetID() { return _ID; }

	void Initialize(unsigned __int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		InterlockedExchange(&_sendFlag, 0);
		InterlockedExchange16(&_validFlag._releaseFlag, 0);

		// ::printf("%d: Session Initialize (%016llx - %016llx)\n", GetCurrentThreadId(), _ID, ID);
		_ID = ID;
		// LeaveLog(100, ID, _validFlag._useCount, _validFlag._releaseFlag);

		_disconnect = false;
		_sock = sock;
		_addr = addr;

		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

	void Terminate()
	{
		// ::printf("%d: Session Terminate (%016llx - %016llx)\n", GetCurrentThreadId(), _ID, (__int64)-1);
		_ID = MAXULONGLONG;
		_disconnect = true;
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
	unsigned __int64 _ID = MAXULONGLONG;

public:
	bool _disconnect = true;
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

	volatile ValidFlag _validFlag;

	/*
public:
#define DEBUG_MAX 1000

	struct DebugData
	{
		int _line = 0;
		__int64 _reqID = 0;
		short _useCount = 0;
		short _releaseFlag = 0;
		int _call = 0;
	};

	volatile long g_idx = -1;
	DebugData debugs[DEBUG_MAX];
	void LeaveLog(int line, __int64 reqID, short useCount, short releaseFlag, int call = 0)
	{
		long idx = InterlockedIncrement(&g_idx);
		debugs[idx % DEBUG_MAX]._line = line;
		debugs[idx % DEBUG_MAX]._reqID = reqID;
		debugs[idx % DEBUG_MAX]._useCount = useCount;
		debugs[idx % DEBUG_MAX]._releaseFlag = releaseFlag;
		debugs[idx % DEBUG_MAX]._call = call;
	}
	*/
};