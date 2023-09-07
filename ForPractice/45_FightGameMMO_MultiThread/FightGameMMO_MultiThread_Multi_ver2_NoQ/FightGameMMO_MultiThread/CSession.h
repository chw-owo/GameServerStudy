#pragma once
#include "CPacket.h"
#include "CRingBuffer.h"
#include <ws2tcpip.h>
#include <synchapi.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

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
	CSession() {}
	CSession(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		_alive = true;
		
		_ID = ID;
		_sock = sock;
		_addr = addr;
		_IOCount = 0;
		_sendFlag = 0;
	
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));

		/*
		_debugLineQ.reserve(10000);
		_debugIOCountQ.reserve(10000);
		_debugFlagQ.reserve(10000);
		_debugUseSizeQ.reserve(10000);
		_debugThreadIDQ.reserve(10000);
		_debugMinQ.reserve(10000);
		_debugSecQ.reserve(10000);
		*/

		InitializeCriticalSection(&_cs);
	}

	~CSession()
	{
		_alive = false;
		_ID = -1;
		_IOCount = -1;
		_sendFlag = -1;
	}

public:
	//void Initialize(__int64 ID, SOCKET sock, SOCKADDR_IN addr);

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
	NetworkOverlapped _releaseOvl;

	// For Synchronization
	CRITICAL_SECTION _cs;
	volatile long _IOCount;
	volatile long _sendFlag;

	
public:
	/*
	vector<int> _debugLineQ;
	vector<int> _debugIOCountQ;
	vector<int> _debugFlagQ;
	vector<int> _debugUseSizeQ;
	vector<int> _debugThreadIDQ;
	vector<int> _debugMinQ;
	vector<int> _debugSecQ;
	SRWLOCK _debugQLock;
	*/
	void PushStateForDebug(int line)
	{
		/*
		AcquireSRWLockExclusive(&_debugQLock);
		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		_debugLineQ.push_back(line);
		_debugIOCountQ.push_back(_IOCount);
		_debugFlagQ.push_back(_sendFlag);
		_debugUseSizeQ.push_back(_sendBuf.GetUseSize());
		_debugThreadIDQ.push_back(GetCurrentThreadId());
		_debugMinQ.push_back(stTime.wMinute);
		_debugSecQ.push_back(stTime.wSecond);
		ReleaseSRWLockExclusive(&_debugQLock);
		*/
	}

	void PrintStateForDebug()
	{
		/*
		AcquireSRWLockExclusive(&_debugQLock);

		::printf("\n<%lld>\n",_debugLineQ.size());

		for (int i = 0; i < _debugThreadIDQ.size(); i++)
		{
			::printf("%d: %d, %d, %d (%d - %02d:%02d)\n", 
				_debugLineQ[i], _debugIOCountQ[i], _debugFlagQ[i], 
				_debugUseSizeQ[i], _debugThreadIDQ[i], _debugMinQ[i], _debugSecQ[i]);
		}

		ReleaseSRWLockExclusive(&_debugQLock);
		*/
	}
	
};

