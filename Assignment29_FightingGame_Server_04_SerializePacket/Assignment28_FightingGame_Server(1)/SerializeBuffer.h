#pragma once
#ifndef  __SERIALIZE_BUFFER__
#define  __SERIALIZE_BUFFER__
#include "Typedef.h"

class SerializeBuffer
{
	friend SerializeBuffer& operator << (SerializeBuffer& out, int8 iValue);
	friend SerializeBuffer& operator << (SerializeBuffer& out, int16 iValue);
	friend SerializeBuffer& operator << (SerializeBuffer& out, int32 iValue);
	friend SerializeBuffer& operator << (SerializeBuffer& out, int64 iValue);

	friend SerializeBuffer& operator << (SerializeBuffer& out, uint8 iValue);
	friend SerializeBuffer& operator << (SerializeBuffer& out, uint16 iValue);
	friend SerializeBuffer& operator << (SerializeBuffer& out, uint32 iValue);
	friend SerializeBuffer& operator << (SerializeBuffer& out, uint64 iValue);

	friend SerializeBuffer& operator >> (SerializeBuffer& out, int8& iValue);
	friend SerializeBuffer& operator >> (SerializeBuffer& out, int16& iValue);
	friend SerializeBuffer& operator >> (SerializeBuffer& out, int32& iValue);
	friend SerializeBuffer& operator >> (SerializeBuffer& out, int64& iValue);

	friend SerializeBuffer& operator >> (SerializeBuffer& out, uint8& iValue);
	friend SerializeBuffer& operator >> (SerializeBuffer& out, uint16& iValue);
	friend SerializeBuffer& operator >> (SerializeBuffer& out, uint32& iValue);
	friend SerializeBuffer& operator >> (SerializeBuffer& out, uint64& iValue);

public:

	enum en_BUFFER
	{
		// 패킷의 기본 버퍼 사이즈.
		eBUFFER_DEFAULT = 1024,
		eBUFFER_MAX = 4096
	};

	SerializeBuffer();
	SerializeBuffer(int iBufferSize);
	virtual	~SerializeBuffer();

	void Clear(void);
	int Resize(int iBufferSize);
	bool IsEmpty(void) { return (_iWritePos == _iReadPos); }
	int	GetBufferSize(void) { return _iBufferSize; }
	int	GetDataSize(void);

	char* GetWritePtr(void) { return &_chpBuffer[_iWritePos]; }
	char* GetReadPtr(void) { return &_chpBuffer[_iReadPos]; }
	int	MoveWritePos(int iSize);
	int	MoveReadPos(int iSize);

	SerializeBuffer& operator = (SerializeBuffer& clSrSerializeBuffer);

	SerializeBuffer& operator << (float fValue);
	SerializeBuffer& operator << (double dValue);
	SerializeBuffer& operator >> (float& fValue);
	SerializeBuffer& operator >> (double& dValue);

	/*
	SerializeBuffer& operator << (char chValue);
	SerializeBuffer& operator << (unsigned char byValue);

	SerializeBuffer& operator << (short shValue);
	SerializeBuffer& operator << (unsigned short wValue);

	SerializeBuffer& operator << (int iValue);
	SerializeBuffer& operator << (long lValue);
	SerializeBuffer& operator << (__int64 iValue);


	SerializeBuffer& operator >> (char& chValue);
	SerializeBuffer& operator >> (BYTE& byValue);

	SerializeBuffer& operator >> (short& shValue);
	SerializeBuffer& operator >> (WORD& wValue);

	SerializeBuffer& operator >> (int& iValue);
	SerializeBuffer& operator >> (DWORD& dwValue);
	SerializeBuffer& operator >> (__int64& iValue);
	*/

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
