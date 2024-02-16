#pragma once
#include "Config.h"

/*
<사용 방법>

1. CLanRecvPacket::Alloc()로 할당, 데이터 삽입 전 Clear()로 초기화.

2. SendLanPacket으로 전송 요청 하기 전 AddUsageCount(n)으로 목적지 수 설정
   Unicast의 경우 1, Multicast의 경우 목적지 수를 n으로 입력한다.

*/

#ifdef LANSERVER

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "ErrorCode.h"
#include "Config.h"
#include "CTlsPool.h"
#include <windows.h>
#include <stdio.h>

class CLanGroup;
class CLanMsg;
class CLanRecvPacket
{
	friend CTlsPool<CLanRecvPacket>;
	friend CLanMsg;

public:
	static CTlsPool<CLanRecvPacket> _pool;

	inline static CLanRecvPacket* Alloc()
	{
		CLanRecvPacket* packet = _pool.Alloc();
		packet->Clear();
		return packet;
	}

	inline static bool Free(CLanRecvPacket* packet)
	{
		if (InterlockedDecrement(&packet->_usageCount) == 0)
		{
			_pool.Free(packet);
			return true;
		}
		return false;
	}

	inline void AddUsageCount(long usageCount)
	{
		InterlockedAdd(&_usageCount, usageCount);
	}

	void SetGroup(CLanGroup* pGroup)
	{
		_pGroup = pGroup;
	}

public:
	inline static int GetPoolSize()
	{
		return _pool.GetPoolSize();
	}

	inline static int GetNodeCount()
	{
		return _pool.GetNodeCount();
	}

protected:
	inline CLanRecvPacket()
		: _iBufferSize(dfPACKET_DEF_SIZE), _iPayloadSize(0), _iHeaderSize(dfLANHEADER_LEN),
		_iPayloadReadPos(dfLANHEADER_LEN), _iPayloadWritePos(dfLANHEADER_LEN),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	inline CLanRecvPacket(int iBufferSize)
		: _iBufferSize(iBufferSize), _iPayloadSize(0), _iHeaderSize(dfLANHEADER_LEN),
		_iPayloadReadPos(dfLANHEADER_LEN), _iPayloadWritePos(dfLANHEADER_LEN),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	inline ~CLanRecvPacket()
	{
		delete[] _chpBuffer;
	}

public:
	inline int GetBufferSize(void) { return _iBufferSize; }
	inline int GetLanPacketSize(void) { return _iPayloadWritePos - _iHeaderReadPos; }
	inline char* GetLanPacketReadPtr(void) { return &_chpBuffer[0]; }

	inline bool IsPayloadEmpty(void) { return (_iPayloadWritePos == _iPayloadReadPos); }
	inline bool IsHeaderEmpty(void) { return (_iHeaderWritePos == _iHeaderReadPos); }
	inline bool IsEmpty(void) { return (IsPayloadEmpty() && IsHeaderEmpty()); }

	inline short GetPayloadSize(void) { return (short)(_iPayloadWritePos - _iPayloadReadPos); }
	inline char* GetPayloadPtr(void) { return &_chpBuffer[dfLANHEADER_LEN]; }
	inline char* GetPayloadReadPtr(void) { return &_chpBuffer[_iPayloadReadPos]; }
	inline int GetPayloadReadPos(void) { return _iPayloadReadPos; }
	inline char* GetPayloadWritePtr(void) { return &_chpBuffer[_iPayloadWritePos]; }
	inline int GetPayloadWritePos(void) { return _iPayloadWritePos; }
	inline short GetHeaderSize(void) { return (short)(_iHeaderWritePos - _iHeaderReadPos); }
	inline char* GetHeaderReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }
	inline char* GetHeaderWritePtr(void) { return &_chpBuffer[_iHeaderWritePos]; }

public:
	inline void Clear(void)
	{
		_iPayloadReadPos = _iHeaderSize;
		_iPayloadWritePos = _iHeaderSize;
		_iHeaderReadPos = 0;
		_iHeaderWritePos = 0;
		_usageCount = 0;
		_pGroup = nullptr;
	}

	inline int Resize(int iBufferSize)
	{
		if (iBufferSize > dfPACKET_MAX_SIZE)
		{
			_errCode = ERR_RESIZE_OVER_MAX;
			return ERR_PACKET;
		}

		char* chpNewBuffer = new char[iBufferSize];
		memcpy_s(chpNewBuffer, iBufferSize, _chpBuffer, _iBufferSize);
		delete[] _chpBuffer;

		_chpBuffer = chpNewBuffer;
		_iBufferSize = iBufferSize;

		return _iBufferSize;
	}

	inline int MovePayloadReadPos(int iSize)
	{
		if (iSize < 0)
		{
			_errCode = ERR_MOVE_PAYLOAD_READ_UNDER;
			return ERR_PACKET;
		}

		if (_iPayloadReadPos + iSize > _iBufferSize)
		{
			_errCode = ERR_MOVE_PAYLOAD_READ_OVER;
			return ERR_PACKET;
		}

		// ::printf("Read: %d => %d\n", _iPayloadReadPos, _iPayloadReadPos + iSize);
		_iPayloadReadPos += iSize;
		return iSize;
	}

	inline int	MovePayloadWritePos(int iSize)
	{
		if (iSize < 0)
		{
			_errCode = ERR_MOVE_PAYLOAD_WRITE_UNDER;
			return ERR_PACKET;
		}

		if (_iPayloadWritePos + iSize > _iBufferSize)
		{
			_errCode = ERR_MOVE_PAYLOAD_WRITE_OVER;
			return ERR_PACKET;
		}

		// ::printf("Write: %d => %d\n", _iPayloadWritePos, _iPayloadWritePos + iSize);
		_iPayloadWritePos += iSize;
		return iSize;
	}

	inline int MoveHeaderReadPos(int iSize)
	{
		if (iSize < 0)
		{
			_errCode = ERR_MOVE_HEADER_READ_UNDER;
			return ERR_PACKET;
		}

		if (_iHeaderReadPos + iSize > _iHeaderSize)
		{
			_errCode = ERR_MOVE_HEADER_READ_OVER;
			return ERR_PACKET;
		}

		_iHeaderReadPos += iSize;
		return iSize;
	}

	inline int	MoveHeaderWritePos(int iSize)
	{
		if (iSize < 0)
		{
			_errCode = ERR_MOVE_HEADER_WRITE_UNDER;
			return ERR_PACKET;
		}

		if (_iHeaderWritePos + iSize > _iHeaderSize)
		{
			_errCode = ERR_MOVE_HEADER_WRITE_OVER;
			return ERR_PACKET;
		}

		_iHeaderWritePos += iSize;
		return iSize;
	}



public:
	inline CLanRecvPacket& operator = (CLanRecvPacket& clSrCLanRecvPacket)
	{
		*this = clSrCLanRecvPacket;
		return *this;
	}

	inline CLanRecvPacket& operator << (float fValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(fValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&fValue, sizeof(fValue));

		_iPayloadWritePos += sizeof(fValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (double dValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(dValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&dValue, sizeof(dValue));

		_iPayloadWritePos += sizeof(dValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (char chValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(chValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&chValue, sizeof(chValue));

		_iPayloadWritePos += sizeof(chValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (unsigned char byValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(byValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&byValue, sizeof(byValue));

		_iPayloadWritePos += sizeof(byValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (short shValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(shValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&shValue, sizeof(shValue));

		_iPayloadWritePos += sizeof(shValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (unsigned short wValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(wValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&wValue, sizeof(wValue));

		_iPayloadWritePos += sizeof(wValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (int iValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(iValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&iValue, sizeof(iValue));

		_iPayloadWritePos += sizeof(iValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (long lValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(lValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&lValue, sizeof(lValue));

		_iPayloadWritePos += sizeof(lValue);
		return *this;
	}
	inline CLanRecvPacket& operator << (__int64 iValue)
	{
		if (_iBufferSize - _iPayloadWritePos < sizeof(iValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			&iValue, sizeof(iValue));

		_iPayloadWritePos += sizeof(iValue);
		return *this;
	}

	inline CLanRecvPacket& operator >> (float& fValue)
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
	inline CLanRecvPacket& operator >> (double& dValue)
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
	inline CLanRecvPacket& operator >> (char& chValue)
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
	inline CLanRecvPacket& operator >> (BYTE& byValue)
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
	inline CLanRecvPacket& operator >> (wchar_t& szValue)
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
	inline CLanRecvPacket& operator >> (short& shValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(short))
		{
			::printf("\n%d - %d = %d\n",
				_iPayloadWritePos, _iPayloadReadPos, _iPayloadWritePos - _iPayloadReadPos);
			_errCode = ERR_GET_SHORT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&shValue, sizeof(short),
			&_chpBuffer[_iPayloadReadPos], sizeof(short));

		_iPayloadReadPos += sizeof(short);
		return *this;
	}
	inline CLanRecvPacket& operator >> (WORD& wValue)
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
	inline CLanRecvPacket& operator >> (int& iValue)
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
	inline CLanRecvPacket& operator >> (DWORD& dwValue)
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
	inline CLanRecvPacket& operator >> (__int64& iValue)
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

	inline int GetRemainPayloadSize(void)
	{
		return _iBufferSize - _iPayloadWritePos;
	}

public:
	inline void CopyRecvBuf(CLanRecvPacket* origin)
	{
		int useSize = origin->GetPayloadSize();
		if (useSize > 0)
		{
			PutPayloadData(origin->GetPayloadReadPtr(), useSize);
		}
	}

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

public:
	int _errCode;
	volatile long _usageCount = 0;

public:
	CLanGroup* _pGroup;
};


#endif