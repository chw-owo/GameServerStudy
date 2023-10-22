#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "CLockFreePool.h"
#include "ErrorCode.h"
#include "Config.h"
#include <windows.h>

class CPacket
{
	friend CLockFreePool<CPacket>;

private:
	volatile long _usageCount = 0;
	static CLockFreePool<CPacket>* _pPacketPool;

private:
	char* _chpBuffer;
	int	_iBufferSize;
	int	_iPayloadSize;
	int	_iHeaderSize;

private:
	int _iPayloadReadPos;
	int _iPayloadWritePos;
	int _iHeaderReadPos;
	int _iHeaderWritePos;

private:
	int _errCode;

private:
	CPacket() : _iBufferSize(dfRBUFFER_DEF_SIZE), _iPayloadSize(0), _iHeaderSize(dfHEADER_LEN),
		_iPayloadReadPos(dfHEADER_LEN), _iPayloadWritePos(dfHEADER_LEN), _iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	CPacket(int headerLen) : _iBufferSize(dfRBUFFER_DEF_SIZE), _iPayloadSize(0), _iHeaderSize(headerLen),
		_iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen), _iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_pPacketPool = new CLockFreePool<CPacket>(0, false);
		_chpBuffer = new char[_iBufferSize];
	}

	CPacket(int headerLen, int iBufferSize) : _iBufferSize(iBufferSize), _iPayloadSize(0), _iHeaderSize(headerLen),
		_iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen), _iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	~CPacket()
	{
		delete[] _chpBuffer;
	}

public:
	void IncrementUsageCount();
	static CPacket* Alloc();
	static bool Free(CPacket* packet);

public:
	int GetBufferSize(void) { return _iBufferSize; }
	int GetPacketSize(void) { return _iPayloadWritePos - _iHeaderReadPos; }
	char* GetPacketReadPtr(void) { return &_chpBuffer[0]; }

	bool IsPayloadEmpty(void) { return (_iPayloadWritePos == _iPayloadReadPos); }
	bool IsHeaderEmpty(void) { return (_iHeaderWritePos == _iHeaderReadPos); }
	bool IsEmpty(void) { return (IsPayloadEmpty() && IsHeaderEmpty()); }

	short GetPayloadSize(void) { return (short)(_iPayloadWritePos - _iPayloadReadPos); }
	char* GetPayloadReadPtr(void) { return &_chpBuffer[_iPayloadReadPos]; }
	char* GetPayloadWritePtr(void) { return &_chpBuffer[_iPayloadWritePos]; }
	short GetHeaderSize(void) { return (short)(_iHeaderWritePos - _iHeaderReadPos); }
	char* GetHeaderReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }
	char* GetHeaderWritePtr(void) { return &_chpBuffer[_iHeaderWritePos]; }

public:
	void Clear(void);
	int Resize(int iBufferSize);
	int	MovePayloadReadPos(int iSize);
	int	MovePayloadWritePos(int iSize);
	int	MoveHeaderReadPos(int iSize);
	int	MoveHeaderWritePos(int iSize);

public:
	CPacket& operator = (CPacket& clSrCPacket);

	CPacket& operator << (float fValue);
	CPacket& operator << (double dValue);
	CPacket& operator << (char chValue);
	CPacket& operator << (unsigned char byValue);
	CPacket& operator << (short shValue);
	CPacket& operator << (unsigned short wValue);
	CPacket& operator << (int iValue);
	CPacket& operator << (long lValue);
	CPacket& operator << (__int64 iValue);

	CPacket& operator >> (float& fValue);
	CPacket& operator >> (double& dValue);
	CPacket& operator >> (char& chValue);
	CPacket& operator >> (BYTE& byValue);
	CPacket& operator >> (wchar_t& szValue);
	CPacket& operator >> (short& shValue);
	CPacket& operator >> (WORD& wValue);
	CPacket& operator >> (int& iValue);
	CPacket& operator >> (DWORD& dwValue);
	CPacket& operator >> (__int64& iValue);

	int GetPayloadData(char* chpDest, int iSize);
	int PeekPayloadData(char* chpDest, int iSize);
	int PutPayloadData(char* chpSrc, int iSrcSize);
	int GetHeaderData(char* chpDest, int iSize);
	int PeekHeaderData(char* chpDest, int iSize);
	int PutHeaderData(char* chpSrc, int iSrcSize);
};
