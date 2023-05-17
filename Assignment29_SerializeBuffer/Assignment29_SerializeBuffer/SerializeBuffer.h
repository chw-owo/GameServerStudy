#pragma once
#ifndef  __SERIALIZE_BUFFER__
#define  __SERIALIZE_BUFFER__
#include <Windows.h>

class SerializeBuffer
{
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
	char* GetBufferPtr(void) { return _chpBuffer; }
	int	GetDataSize(void);
	int	MoveWritePos(int iSize);
	int	MoveReadPos(int iSize);

	SerializeBuffer& operator = (SerializeBuffer& clSrSerializeBuffer);

	SerializeBuffer& operator << (unsigned char byValue);
	SerializeBuffer& operator << (char chValue);

	SerializeBuffer& operator << (short shValue);
	SerializeBuffer& operator << (unsigned short wValue);

	SerializeBuffer& operator << (int iValue);
	SerializeBuffer& operator << (long lValue);
	SerializeBuffer& operator << (float fValue);

	SerializeBuffer& operator << (__int64 iValue);
	SerializeBuffer& operator << (double dValue);

	SerializeBuffer& operator >> (BYTE& byValue);
	SerializeBuffer& operator >> (char& chValue);

	SerializeBuffer& operator >> (short& shValue);
	SerializeBuffer& operator >> (WORD& wValue);

	SerializeBuffer& operator >> (int& iValue);
	SerializeBuffer& operator >> (DWORD& dwValue);
	SerializeBuffer& operator >> (float& fValue);

	SerializeBuffer& operator >> (__int64& iValue);
	SerializeBuffer& operator >> (double& dValue);

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
