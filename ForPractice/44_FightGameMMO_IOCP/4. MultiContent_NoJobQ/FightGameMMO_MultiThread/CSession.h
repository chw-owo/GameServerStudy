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
		_socketAlive = true;
		
		_ID = ID;
		_sock = sock;
		_addr = addr;
		_IOCount = 0;
		_sendFlag = 0;
	
		_recvOvl._type = NET_TYPE::RECV;
		_sendOvl._type = NET_TYPE::SEND;
		_releaseOvl._type = NET_TYPE::RELEASE;
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));

		InitializeCriticalSection(&_cs);
		InitializeCriticalSection(&_csDebug);
		// _debugDataArray.reserve(1000);
	}

	~CSession()
	{
		_socketAlive = false;
		_ID = -1;
		_IOCount = -1;
		_sendFlag = -1;
	}

public:
	//void Initialize(__int64 ID, SOCKET sock, SOCKADDR_IN addr);

public:
	bool _socketAlive;
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
	
private:
	class CSessionDebugData
	{
		friend CSession;

	private:
		CSessionDebugData() :
			_line(-1), _IOCount(-1), _flag(-1), 
			_sendUseSize(-1), _threadID(-1), _min(-1), _sec(-1){}

		CSessionDebugData(int line, int IOCount, int flag,
			int sendUseSize, int threadID, int min, int sec, bool recv0, bool send0)
			: _line(line), _IOCount(IOCount), _flag(flag),
			_sendUseSize(sendUseSize), _threadID(threadID), _min(min), _sec(sec) {}

	private:
		int _line;
		int _IOCount;
		int _flag;
		int _sendUseSize;
		int _threadID;
		int _min;
		int _sec;
	};

private:
	CRITICAL_SECTION _csDebug;
	vector<CSessionDebugData*> _debugDataArray;

public:
	void PushStateForDebug(int line)
	{
		/*
		EnterCriticalSection(&_csDebug);
		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		
		CSessionDebugData* data = new CSessionDebugData(
			line, _IOCount, _sendFlag ,_sendBuf.GetUseSize(), GetCurrentThreadId(), 
			stTime.wMinute, stTime.wSecond);
		_debugDataArray.push_back(data);

		LeaveCriticalSection(&_csDebug);
		*/
	}

	void PrintStateForDebug()
	{
		/*
		EnterCriticalSection(&_csDebug);
		::wprintf(L"\n<%lld>\n", _debugDataArray.size());
		for (int i = 0; i < _debugDataArray.size(); i++)
		{
			::wprintf(L"%d: %d, %d, %d (%d - %02d:%02d)\n", 
				_debugDataArray[i]->_line, _debugDataArray[i]->_IOCount, 
				_debugDataArray[i]->_flag, _debugDataArray[i]->_sendUseSize,
				_debugDataArray[i]->_threadID, _debugDataArray[i]->_min, _debugDataArray[i]->_sec);
		}
		LeaveCriticalSection(&_csDebug);
		*/
	}
	
};

