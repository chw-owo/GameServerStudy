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
		_disconnect = false;
		_ID = ID;
		_sock = sock;
		_addr = addr;
		_sendFlag = 0;
		_validFlag._flag = 0;

		ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl));
		ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

	void Terminate()
	{
		_disconnect = true;
		_ID = -1;
		_sock = INVALID_SOCKET;
		_sendFlag = 0;
		_validFlag._IOCount = 0;
		_validFlag._releaseFlag = 1;

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

public:
	bool _disconnect = true;
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
			short _releaseFlag;
			short _IOCount;
		};
		long _flag;
	};
	volatile ValidFlag _validFlag;

public:
#define dfSESSION_DEBUG_MAX 1000
	class SessionDebugData // Debug For Initial & Release
	{
	public:
		void LeaveLog(int line, int threadID, short IOCount, short releaseFlag, __int64 sessionID, __int64 reqID)
		{
			_line = line;
			_threadID = threadID;
			_IOCount = IOCount;
			_releaseFlag = releaseFlag;
			_sessionID = sessionID;
			_reqID = reqID;
		}

	private:
		int _line = -1;
		int _threadID = -1;
		short _IOCount = -1;
		short _releaseFlag = -1;
		__int64 _sessionID = -1;
		__int64 _reqID = -1;
	};

public:
	SessionDebugData _debugData[dfSESSION_DEBUG_MAX];
	volatile long _debugIdx = -1;
};

