#pragma once
#include "CNetRecvPacket.h"
#include "CNetSendPacket.h"
#include "Utils.h"
#include <queue>
using namespace std;

class CNetSession
{
public:
	inline CNetSession()
	{
		InitializeSRWLock(&_lock);
		_recvComplOvl._type = NET_TYPE::RECV_COMPLETE;
		_sendComplOvl._type = NET_TYPE::SEND_COMPLETE;
		_sendPostOvl._type = NET_TYPE::SEND_POST;
		_releaseOvl._type = NET_TYPE::RELEASE;

		_recvBuf = CNetRecvPacket::Alloc();
		_recvBuf->AddUsageCount(1);
	}

	inline ~CNetSession()
	{
		CNetRecvPacket::Free(_recvBuf);
	}

public:
	unsigned __int64 GetID() { return _ID; }

	inline void Initialize(unsigned __int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		_recvBuf->Clear();
		_recvBuf->AddUsageCount(1);

		_ID = ID;
		_disconnect = false;
		_sock = sock;
		_addr = addr;

		_IOCount = 0;
		_sendFlag = 0;
		_sendCount = 0;

		ZeroMemory(&_recvComplOvl._ovl, sizeof(_recvComplOvl._ovl));
		ZeroMemory(&_sendComplOvl._ovl, sizeof(_sendComplOvl._ovl));
		ZeroMemory(&_sendPostOvl._ovl, sizeof(_sendPostOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));
	}

	inline void Terminate()
	{
		_ID = MAXULONGLONG;
		_disconnect = true;
		_sock = INVALID_SOCKET;

		while (_sendBuf.size() > 0)
		{
			CNetSendPacket* packet = _sendBuf.front();
			_sendBuf.pop();
			CNetSendPacket::Free(packet);
		}

		while (_tempBuf.size() > 0)
		{
			CNetSendPacket* packet = _tempBuf.front();
			_tempBuf.pop();
			CNetSendPacket::Free(packet);
		}
	}

private:
	unsigned __int64 _ID = MAXULONGLONG;

public:
	bool _disconnect = true;
	volatile long _IOCount;
	volatile long _sendFlag;
	volatile long _sendCount;

	SOCKET _sock;
	SOCKADDR_IN _addr;

	CNetRecvPacket* _recvBuf;
	queue<CNetSendPacket*> _sendBuf;
	queue<CNetSendPacket*> _tempBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvComplOvl;
	NetworkOverlapped _sendComplOvl;
	NetworkOverlapped _sendPostOvl;
	NetworkOverlapped _releaseOvl;

	SRWLOCK _lock;
};