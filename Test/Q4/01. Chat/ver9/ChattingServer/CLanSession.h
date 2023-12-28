#pragma once
#include "CLanPacket.h"
#include "CRingBuffer.h"
#include "Utils.h"

class CLanSession
{
public:
	inline CLanSession()
	{
		_validFlag._flag = 0;
		_recvComplOvl._type = NET_TYPE::RECV_COMPLETE;
		_sendComplOvl._type = NET_TYPE::SEND_COMPLETE;
		_sendPostOvl._type = NET_TYPE::SEND_POST;

		_recvBuf = CLanPacket::Alloc();
		_recvBuf->Clear();
		_recvBuf->AddUsageCount(1);
	}

	inline ~CLanSession()
	{
		CLanPacket::Free(_recvBuf);
	}

public:
	unsigned __int64 GetID() { return _ID; }

	inline void Initialize(unsigned __int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		InterlockedExchange(&_sendFlag, 0);
		InterlockedExchange16(&_validFlag._releaseFlag, 0);

		_ID = ID;
		_disconnect = false;
		_sock = sock;
		_addr = addr;

		ZeroMemory(&_recvComplOvl._ovl, sizeof(_recvComplOvl._ovl));
		ZeroMemory(&_sendComplOvl._ovl, sizeof(_sendComplOvl._ovl));
		ZeroMemory(&_sendPostOvl._ovl, sizeof(_sendPostOvl._ovl));

		_sendBuf.ClearBuffer();
		_saveBuf.ClearBuffer();
	}

	inline void Terminate()
	{
		_ID = MAXULONGLONG;
		_disconnect = true;
		_sock = INVALID_SOCKET;

		while (_sendBuf.GetUseSize() > 0)
		{
			CLanPacket* packet;
			_sendBuf.Dequeue((char*)&packet, sizeof(CLanPacket*));
			CLanPacket::Free(packet);
		}
		while (_saveBuf.GetUseSize() > 0)
		{
			CLanPacket* packet;
			_saveBuf.Dequeue((char*)&packet, sizeof(CLanPacket*));
			CLanPacket::Free(packet);
		}

		_sendBuf.ClearBuffer();
		_saveBuf.ClearBuffer();
	}

private:
	unsigned __int64 _ID = MAXULONGLONG;

public:
	bool _disconnect = true;
	volatile long _sendFlag;
	volatile long _sendCount;

	SOCKET _sock;
	SOCKADDR_IN _addr;

	CLanPacket* _recvBuf;
	CRingBuffer _sendBuf;
	CRingBuffer _saveBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvComplOvl;
	NetworkOverlapped _sendComplOvl;
	NetworkOverlapped _sendPostOvl;

	volatile ValidFlag _validFlag;
};