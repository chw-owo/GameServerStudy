#pragma once
#include "CNetPacket.h"
#include "CLockFreeQueue.h"
#include "Utils.h"

class CNetSession
{
public:
	CNetSession()
	{
		_validFlag._flag = 0;
		_recvComplOvl._type = NET_TYPE::RECV_COMPLETE;
		_sendComplOvl._type = NET_TYPE::SEND_COMPLETE;
		_recvPostOvl._type = NET_TYPE::RECV_POST;
		_sendPostOvl._type = NET_TYPE::SEND_POST;
		_releaseOvl._type = NET_TYPE::RELEASE;

		_recvBuf = CNetPacket::Alloc();
		_recvBuf->Clear();
		_recvBuf->AddUsageCount(1);
	}

	~CNetSession()
	{
		CNetPacket::Free(_recvBuf);
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
			CNetPacket* packet = _sendBuf.Dequeue();
			CNetPacket::Free(packet);
		}
		while (_tempBuf.GetUseSize() > 0)
		{
			CNetPacket* packet = _tempBuf.Dequeue();
			CNetPacket::Free(packet);
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

	CNetPacket* _recvBuf;
	CLockFreeQueue<CNetPacket*> _sendBuf;
	CLockFreeQueue<CNetPacket*> _tempBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvComplOvl;
	NetworkOverlapped _sendComplOvl;
	NetworkOverlapped _recvPostOvl;
	NetworkOverlapped _sendPostOvl;
	NetworkOverlapped _releaseOvl;

	volatile ValidFlag _validFlag;
};