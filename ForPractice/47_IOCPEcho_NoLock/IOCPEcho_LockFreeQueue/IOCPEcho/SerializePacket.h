#pragma once
#ifndef  __SERIALIZE_BUFFER__
#define  __SERIALIZE_BUFFER__

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include "Protocol.h"

class SerializePacket
{
public:

	enum en_BUFFER
	{
		// 패킷의 기본 버퍼 사이즈.
		eBUFFER_DEFAULT = 1024,
		eBUFFER_MAX = 4096
	};

	SerializePacket();
	SerializePacket(int iBufferSize);
	virtual	~SerializePacket();

	void Clear(void);
	int Resize(int iBufferSize);
	int	GetBufferSize(void) { return _iBufferSize; }

	bool IsPayloadEmpty(void) { return (_iPayloadWritePos == _iPayloadReadPos); }
	bool IsHeaderEmpty(void) { return (_iHeaderWritePos == _iHeaderReadPos); }
	bool IsEmpty(void) { return (IsPayloadEmpty() && IsHeaderEmpty()); }
	
	int	GetPayloadSize(void) { return _iPayloadWritePos - _iPayloadReadPos; }
	char* GetPayloadReadPtr(void) { return &_chpBuffer[_iPayloadReadPos]; }
	char* GetPayloadWritePtr(void) { return &_chpBuffer[_iPayloadWritePos]; }
	int	MovePayloadReadPos(int iSize);
	int	MovePayloadWritePos(int iSize);

	int GetHeaderSize(void) { return _iHeaderWritePos - _iHeaderReadPos; }
	char* GetHeaderReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }
	char* GetHeaderWritePtr(void) { return &_chpBuffer[_iHeaderWritePos]; }
	int	MoveHeaderReadPos(int iSize);
	int	MoveHeaderWritePos(int iSize);

	int GetPacketSize(void) { return _iPayloadWritePos - _iHeaderReadPos; }
	char* GetPacketReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }

	SerializePacket& operator = (SerializePacket& clSrSerializePacket);

	SerializePacket& operator << (float fValue);
	SerializePacket& operator << (double dValue);
	SerializePacket& operator >> (float& fValue);
	SerializePacket& operator >> (double& dValue);

	SerializePacket& operator << (char chValue);
	SerializePacket& operator << (unsigned char byValue);

	SerializePacket& operator << (short shValue);
	SerializePacket& operator << (unsigned short wValue);

	SerializePacket& operator << (int iValue);
	SerializePacket& operator << (long lValue);
	SerializePacket& operator << (__int64 iValue);


	SerializePacket& operator >> (char& chValue);
	SerializePacket& operator >> (BYTE& byValue);

	SerializePacket& operator >> (wchar_t& szValue);
	SerializePacket& operator >> (short& shValue);
	SerializePacket& operator >> (WORD& wValue);

	SerializePacket& operator >> (int& iValue);
	SerializePacket& operator >> (DWORD& dwValue);
	SerializePacket& operator >> (__int64& iValue);

	int	GetPayloadData(char* chpDest, int iSize);
	int	PeekPayloadData(char* chpDest, int iSize);
	int	PutPayloadData(char* chpSrc, int iSrcSize);

	int	GetHeaderData(char* chpDest, int iSize);
	int	PeekHeaderData(char* chpDest, int iSize);
	int PutHeaderData(char* chpSrc, int iSrcSize);

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
};

#endif
