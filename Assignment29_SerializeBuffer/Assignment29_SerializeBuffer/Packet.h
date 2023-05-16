#pragma once
#ifndef  __PACKET__
#define  __PACKET__
#include <Windows.h>

class CPacket
{
public:

	enum en_PACKET
	{
		// 패킷의 기본 버퍼 사이즈.
		eBUFFER_DEFAULT = 1024,
		eBUFFER_MAX = 4096
	};

	CPacket();
	CPacket(int iBufferSize);
	virtual	~CPacket();
	void Clear(void);
	int Resize(int iBufferSize);

	bool IsEmpty(void) { return (_iWritePos == _iReadPos); }
	int	GetBufferSize(void) { return _iBufferSize; }
	char* GetBufferPtr(void) { return _chpBuffer; }
	int	GetDataSize(void);
	int	MoveWritePos(int iSize);
	int	MoveReadPos(int iSize);

	CPacket& operator = (CPacket& clSrcPacket);

	CPacket& operator << (unsigned char byValue);
	CPacket& operator << (char chValue);

	CPacket& operator << (short shValue);
	CPacket& operator << (unsigned short wValue);

	CPacket& operator << (int iValue);
	CPacket& operator << (long lValue);
	CPacket& operator << (float fValue);

	CPacket& operator << (__int64 iValue);
	CPacket& operator << (double dValue);

	CPacket& operator >> (BYTE& byValue);
	CPacket& operator >> (char& chValue);

	CPacket& operator >> (short& shValue);
	CPacket& operator >> (WORD& wValue);

	CPacket& operator >> (int& iValue);
	CPacket& operator >> (DWORD& dwValue);
	CPacket& operator >> (float& fValue);

	CPacket& operator >> (__int64& iValue);
	CPacket& operator >> (double& dValue);

	int	GetData(char* chpDest, int iSize);
	int	PutData(char* chpSrc, int iSrcSize);

protected:
	int	_iBufferSize;
	int	_iDataSize;

private:
	char* _chpBuffer;
	int _iReadPos;
	int _iWritePos;

};

#endif
