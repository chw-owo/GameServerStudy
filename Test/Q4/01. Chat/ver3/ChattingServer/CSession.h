
#pragma once
#include "Config.h"
#include "CPacket.h"
#include "CPacket.h"
#include "CLockFreeQueue.h"

#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

enum class NET_TYPE
{
	SEND_COMPLETE = 0,
	RECV_COMPLETE,
	SEND_POST,
	RECV_POST,
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
		_recvComplOvl._type = NET_TYPE::RECV_COMPLETE;
		_sendComplOvl._type = NET_TYPE::SEND_COMPLETE;
		_recvPostOvl._type = NET_TYPE::RECV_POST;
		_sendPostOvl._type = NET_TYPE::SEND_POST;
		_releaseOvl._type = NET_TYPE::RELEASE;

		_recvBuf = CPacket::Alloc();
		_recvBuf->Clear();
		_recvBuf->AddUsageCount(1);
	}

	~CSession()
	{
		CPacket::Free(_recvBuf);
	}

public:
	unsigned __int64 GetID() { return _ID; }

	void Initialize(unsigned __int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		InterlockedExchange(&_sendFlag, 0);
		InterlockedExchange16(&_validFlag._releaseFlag, 0);

		_ID = ID;
		_disconnect = false;
		_sock = sock;
		_addr = addr;

		ZeroMemory(&_recvComplOvl._ovl, sizeof(_recvComplOvl._ovl));
		ZeroMemory(&_sendComplOvl._ovl, sizeof(_sendComplOvl._ovl));
		ZeroMemory(&_recvPostOvl._ovl, sizeof(_recvPostOvl._ovl));
		ZeroMemory(&_sendPostOvl._ovl, sizeof(_sendPostOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));

		
	}

	void Terminate()
	{
		_ID = MAXULONGLONG;
		_disconnect = true;
		_sock = INVALID_SOCKET;

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

	CPacket* _recvBuf;
	CLockFreeQueue<CPacket*> _sendBuf;
	CLockFreeQueue<CPacket*> _tempBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvComplOvl;
	NetworkOverlapped _sendComplOvl;
	NetworkOverlapped _recvPostOvl;
	NetworkOverlapped _sendPostOvl;
	NetworkOverlapped _releaseOvl;

	volatile ValidFlag _validFlag;
};