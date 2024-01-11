#pragma once
#include "CNetRecvPacket.h"
#include "CNetSendPacket.h"
#include "CLockFreeQueue.h"
#include "CRingBuffer.h"
#include "Utils.h"

class CNetSession
{
public:
	inline CNetSession()
	{
		_tempBuf = new CRingBuffer(dfTEMPBUF_SIZE);
		_OnRecvQ = new CRingBuffer(dfONRECVQ_SIZE);

		_validFlag._flag = 0;
		InitializeCriticalSection(&_groupLock);

		_recvComplOvl._type = NET_TYPE::RECV_COMPLETE;
		_sendComplOvl._type = NET_TYPE::SEND_COMPLETE;
		_sendPostOvl._type = NET_TYPE::SEND_POST;
		_releaseOvl._type = NET_TYPE::RELEASE;

		_recvBuf = CNetRecvPacket::Alloc();
		_recvBuf->AddUsageCount(1);
	}

	inline ~CNetSession()
	{
		delete _tempBuf;
		delete _OnRecvQ;
		CNetRecvPacket::Free(_recvBuf);
	}

public:
	unsigned __int64 GetID() { return _ID; }

	inline void Initialize(unsigned __int64 ID, SOCKET sock, SOCKADDR_IN addr)
	{
		_tempBuf->ClearBuffer();
		_OnRecvQ->ClearBuffer();

		InterlockedExchange(&_sendFlag, 0);
		InterlockedExchange16(&_validFlag._releaseFlag, 0);

		_ID = ID;
		_disconnect = false;
		_sock = sock;
		_addr = addr;

		EnterCriticalSection(&_groupLock);
		_pGroup = nullptr;
		LeaveCriticalSection(&_groupLock);

		ZeroMemory(&_recvComplOvl._ovl, sizeof(_recvComplOvl._ovl));
		ZeroMemory(&_sendComplOvl._ovl, sizeof(_sendComplOvl._ovl));
		ZeroMemory(&_sendPostOvl._ovl, sizeof(_sendPostOvl._ovl));
		ZeroMemory(&_releaseOvl._ovl, sizeof(_releaseOvl._ovl));

		while (_sendBuf.GetUseSize() > 0)
		{
			CNetSendPacket* packet = _sendBuf.Dequeue();
			CNetSendPacket::Free(packet);
		}
	}

	inline void Terminate()
	{
		_ID = MAXULONGLONG;
		_disconnect = true;
		_sock = INVALID_SOCKET;

		EnterCriticalSection(&_groupLock);
		_pGroup = nullptr;
		LeaveCriticalSection(&_groupLock);
	}

private:
	unsigned __int64 _ID = MAXULONGLONG;

public:
	bool _disconnect = true;
	volatile long _sendFlag = 0;
	volatile long _sendCount = 0;

	SOCKET _sock;
	SOCKADDR_IN _addr;

	CNetRecvPacket* _recvBuf;
	CLockFreeQueue<CNetSendPacket*> _sendBuf;
	CRingBuffer* _tempBuf;
	WSABUF _wsaRecvbuf[dfWSARECVBUF_CNT];
	WSABUF _wsaSendbuf[dfWSASENDBUF_CNT];

	NetworkOverlapped _recvComplOvl;
	NetworkOverlapped _sendComplOvl;
	NetworkOverlapped _sendPostOvl;
	NetworkOverlapped _releaseOvl;

	volatile ValidFlag _validFlag;

public: // For Group
	CNetGroup* _pGroup = nullptr;
	CRITICAL_SECTION _groupLock;
	CRingBuffer* _OnRecvQ;
};