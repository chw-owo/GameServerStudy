#pragma once

/*
<사용 방법>

1. CPacket::Alloc()로 할당, 데이터 삽입 전 Clear()로 초기화.

2. SendPacket으로 전송 요청 하기 전 AddUsageCount(n)으로 목적지 수 설정
   Unicast의 경우 1, Multicast의 경우 목적지 수를 n으로 입력한다.

*/

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "CLockFreePool.h"
#include "ErrorCode.h"
#include "Config.h"
#include <windows.h>
#include <stdio.h>

class CPacket
{
	friend CLockFreePool<CPacket>;

public:
	static CLockFreePool<CPacket> _pool;

	static CPacket* Alloc()
	{
		CPacket* packet = _pool.Alloc();
		return packet;
	}

	static bool Free(CPacket* packet)
	{
		if (InterlockedDecrement(&packet->_usageCount) == 0)
		{
			_pool.Free(packet);
			return true;
		}
		return false;
	}

	static int GetPoolSize()
	{
		return _pool.GetPoolSize();
	}

	static int GetNodeCount()
	{
		return _pool.GetNodeCount();
	}

#define dfMAX 100
	void AddUsageCount(long usageCount)
	{
		InterlockedAdd(&_usageCount, usageCount);
	}

private:
	inline CPacket()
		: _iBufferSize(dfRBUFFER_DEF_SIZE), _iPayloadSize(0), _iHeaderSize(dfHEADER_LEN),
		_iPayloadReadPos(dfHEADER_LEN), _iPayloadWritePos(dfHEADER_LEN),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	inline CPacket(int headerLen)
		: _iBufferSize(dfRBUFFER_DEF_SIZE), _iPayloadSize(0), _iHeaderSize(headerLen),
		_iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	inline CPacket(int headerLen, int iBufferSize)
		: _iBufferSize(iBufferSize), _iPayloadSize(0), _iHeaderSize(headerLen),
		_iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	inline ~CPacket()
	{
		delete[] _chpBuffer;
	}

public:
	volatile long _encode = 0;
#ifdef NETSERVER
	bool Decode(stHeader& header)
	{	
		unsigned char checkSum = 0;
		unsigned char* payload = (unsigned char*) GetPayloadPtr();

		char e_cur = header._checkSum;
		char p_cur = e_cur ^ (dfPACKET_KEY + 1);
		header._checkSum = p_cur ^ (header._randKey + 1);
		char e_prev = e_cur;
		char p_prev = p_cur;	

		for (int i = 0; i < header._len; i++)
		{
			e_cur = payload[i];
			p_cur = e_cur ^ (e_prev + dfPACKET_KEY + 2 + i);
			payload[i] = p_cur ^ (p_prev + header._randKey + 2 + i);
			checkSum += payload[i];

			e_prev = e_cur;
			p_prev = p_cur;
		}

		checkSum = checkSum % 256;
		if (header._checkSum != checkSum) 
		{
			::printf("%02x != %02x\n", header._checkSum, checkSum);
			return false;
		}

		return true;
	}

	bool Encode(stHeader& header) 
	{
		if (InterlockedExchange(&_encode, 1) == 1) return false;
		// 콘텐츠를 멀티로 제작할 경우 event를 통한 완료 확인 및 대기 필요

		unsigned char checkSum = 0;
		char* payload = GetPayloadPtr();
		for (int i = 0; i < header._len; i++)
		{
			checkSum += payload[i];
		}
		header._checkSum = checkSum % 256;

		char d = header._checkSum;
		char p = d ^ (header._randKey + 1);
		char e = p ^ (dfPACKET_KEY + 1);
		header._checkSum = e;

		for (int i = 0; i < header._len; i++)
		{
			d = payload[i];
			p = d ^ (p + header._randKey + 2 + i);
			e = p ^ (e + dfPACKET_KEY + 2 + i);
			payload[i] = e;
		}
		
		return true;
	}
#endif

public:
	inline int GetBufferSize(void) { return _iBufferSize; }
	inline int GetPacketSize(void) { return _iPayloadWritePos - _iHeaderReadPos; }
	inline char* GetPacketReadPtr(void) { return &_chpBuffer[0]; }

	inline bool IsPayloadEmpty(void) { return (_iPayloadWritePos == _iPayloadReadPos); }
	inline bool IsHeaderEmpty(void) { return (_iHeaderWritePos == _iHeaderReadPos); }
	inline bool IsEmpty(void) { return (IsPayloadEmpty() && IsHeaderEmpty()); }

	inline short GetPayloadSize(void) { return (short)(_iPayloadWritePos - _iPayloadReadPos); }
	inline char* GetPayloadPtr(void) { return &_chpBuffer[dfHEADER_LEN]; }
	inline char* GetPayloadReadPtr(void) { return &_chpBuffer[_iPayloadReadPos]; }
	inline char* GetPayloadWritePtr(void) { return &_chpBuffer[_iPayloadWritePos]; }
	inline short GetHeaderSize(void) { return (short)(_iHeaderWritePos - _iHeaderReadPos); }
	inline char* GetHeaderReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }
	inline char* GetHeaderWritePtr(void) { return &_chpBuffer[_iHeaderWritePos]; }

public:
	void Clear(void);
	int Resize(int iBufferSize);
	int	MovePayloadReadPos(int iSize);
	int	MovePayloadWritePos(int iSize);
	int	MoveHeaderReadPos(int iSize);
	int	MoveHeaderWritePos(int iSize);

public:
	inline CPacket& operator = (CPacket& clSrCPacket)
	{
		*this = clSrCPacket;
		return *this;
	}

	inline CPacket& operator << (float fValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(fValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&fValue, sizeof(fValue));

		_iPayloadWritePos += sizeof(fValue);
		return *this;
	}
	inline CPacket& operator << (double dValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(dValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&dValue, sizeof(dValue));

		_iPayloadWritePos += sizeof(dValue);
		return *this;
	}
	inline CPacket& operator << (char chValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(chValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&chValue, sizeof(chValue));

		_iPayloadWritePos += sizeof(chValue);
		return *this;
	}
	inline CPacket& operator << (unsigned char byValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(byValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&byValue, sizeof(byValue));

		_iPayloadWritePos += sizeof(byValue);
		return *this;
	}
	inline CPacket& operator << (short shValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(shValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&shValue, sizeof(shValue));

		_iPayloadWritePos += sizeof(shValue);
		return *this;
	}
	inline CPacket& operator << (unsigned short wValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(wValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&wValue, sizeof(wValue));

		_iPayloadWritePos += sizeof(wValue);
		return *this;
	}
	inline CPacket& operator << (int iValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(iValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&iValue, sizeof(iValue));

		_iPayloadWritePos += sizeof(iValue);
		return *this;
	}
	inline CPacket& operator << (long lValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(lValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&lValue, sizeof(lValue));

		_iPayloadWritePos += sizeof(lValue);
		return *this;
	}
	inline CPacket& operator << (__int64 iValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(iValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&iValue, sizeof(iValue));

		_iPayloadWritePos += sizeof(iValue);
		return *this;
	}

	inline CPacket& operator >> (float& fValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(float))
		{
			_errCode = ERR_GET_FLOAT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&fValue, sizeof(float),
			&_chpBuffer[_iPayloadReadPos], sizeof(float));

		_iPayloadReadPos += sizeof(float);
		return *this;
	}
	inline CPacket& operator >> (double& dValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(double))
		{
			_errCode = ERR_GET_DOUBLE_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&dValue, sizeof(double),
			&_chpBuffer[_iPayloadReadPos], sizeof(double));

		_iPayloadReadPos += sizeof(double);
		return *this;
	}
	inline CPacket& operator >> (char& chValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(char))
		{
			_errCode = ERR_GET_CHAR_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&chValue, sizeof(char),
			&_chpBuffer[_iPayloadReadPos], sizeof(char));

		_iPayloadReadPos += sizeof(char);
		return *this;
	}
	inline CPacket& operator >> (BYTE& byValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(BYTE))
		{
			_errCode = ERR_GET_BYTE_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&byValue, sizeof(BYTE),
			&_chpBuffer[_iPayloadReadPos], sizeof(BYTE));

		_iPayloadReadPos += sizeof(BYTE);
		return *this;
	}
	inline CPacket& operator >> (wchar_t& szValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(wchar_t))
		{
			_errCode = ERR_GET_WCHAR_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&szValue, sizeof(wchar_t),
			&_chpBuffer[_iPayloadReadPos], sizeof(wchar_t));

		_iPayloadReadPos += sizeof(wchar_t);
		return *this;
	}
	inline CPacket& operator >> (short& shValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(short))
		{
			_errCode = ERR_GET_SHORT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&shValue, sizeof(short),
			&_chpBuffer[_iPayloadReadPos], sizeof(short));

		_iPayloadReadPos += sizeof(short);
		return *this;
	}
	inline CPacket& operator >> (WORD& wValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(WORD))
		{
			_errCode = ERR_GET_WORD_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&wValue, sizeof(WORD),
			&_chpBuffer[_iPayloadReadPos], sizeof(WORD));

		_iPayloadReadPos += sizeof(WORD);
		return *this;
	}
	inline CPacket& operator >> (int& iValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(int))
		{
			_errCode = ERR_GET_INT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&iValue, sizeof(int),
			&_chpBuffer[_iPayloadReadPos], sizeof(int));

		_iPayloadReadPos += sizeof(int);
		return *this;
	}
	inline CPacket& operator >> (DWORD& dwValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(DWORD))
		{
			_errCode = ERR_GET_DWORD_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&dwValue, sizeof(DWORD),
			&_chpBuffer[_iPayloadReadPos], sizeof(DWORD));

		_iPayloadReadPos += sizeof(DWORD);
		return *this;
	}
	inline CPacket& operator >> (__int64& iValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(__int64))
		{
			_errCode = ERR_GET_INT64_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&iValue, sizeof(__int64),
			&_chpBuffer[_iPayloadReadPos], sizeof(__int64));

		_iPayloadReadPos += sizeof(__int64);
		return *this;
	}

	inline int GetPayloadData(char* chpDest, int iSize)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < iSize)
		{
			_errCode = ERR_GET_PAYLOAD_OVER;
			return ERR_PACKET;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iPayloadReadPos], iSize);

		_iPayloadReadPos += iSize;
		return iSize;
	}
	inline int PeekPayloadData(char* chpDest, int iSize)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < iSize)
		{
			_errCode = ERR_PEEK_PAYLOAD_OVER;
			return ERR_PACKET;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iPayloadReadPos], iSize);
		return iSize;
	}
	inline int PutPayloadData(char* chpSrc, int iSrcSize)
	{
		if (_iBufferSize - _iPayloadWritePos < iSrcSize)
			Resize(_iBufferSize + (int)(iSrcSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			chpSrc, iSrcSize);

		_iPayloadWritePos += iSrcSize;
		return iSrcSize;
	}

	inline int GetHeaderData(char* chpDest, int iSize)
	{
		if (_iHeaderSize < iSize)
		{
			_errCode = ERR_GET_HEADER_OVER_MAX;
			return ERR_PACKET;
		}

		if (_iHeaderWritePos - _iHeaderReadPos < iSize)
		{
			_errCode = ERR_GET_HEADER_OVER_EMPTY;
			return ERR_PACKET;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);

		_iHeaderReadPos += iSize;
		return iSize;
	}
	inline int PeekHeaderData(char* chpDest, int iSize)
	{
		if (_iHeaderSize < iSize)
		{
			_errCode = ERR_PEEK_HEADER_OVER_MAX;
			return ERR_PACKET;
		}

		if (_iHeaderWritePos - _iHeaderReadPos < iSize)
		{
			_errCode = ERR_PEEK_HEADER_OVER_EMPTY;
			return ERR_PACKET;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);
		return iSize;
	}
	inline int PutHeaderData(char* chpSrc, int iSrcSize)
	{
		if (_iHeaderSize < _iHeaderWritePos + iSrcSize)
		{
			_errCode = ERR_PUT_HEADER_OVER;
			return ERR_PACKET;
		}

		memcpy_s(&_chpBuffer[_iHeaderWritePos],
			_iHeaderSize, chpSrc, iSrcSize);

		_iHeaderWritePos += iSrcSize;

		return iSrcSize;
	}

private:
	volatile long _usageCount;

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
};
