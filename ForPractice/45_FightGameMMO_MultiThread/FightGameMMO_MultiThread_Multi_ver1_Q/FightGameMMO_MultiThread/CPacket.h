#pragma once
#ifndef  __SERIALIZE_BUFFER__
#define  __SERIALIZE_BUFFER__

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include "Protocol.h"
#include <stdio.h>

class CPacket
{
public:

	enum en_BUFFER
	{
		// 패킷의 기본 버퍼 사이즈.
		eBUFFER_DEFAULT = 1024,
		eBUFFER_MAX = 4096
	};

	CPacket()
		: _iBufferSize(eBUFFER_DEFAULT), _iPayloadSize(0), _iHeaderSize(dfHEADER_LEN),
		_iPayloadReadPos(dfHEADER_LEN), _iPayloadWritePos(dfHEADER_LEN),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	CPacket(int headerLen)
		: _iBufferSize(eBUFFER_DEFAULT), _iPayloadSize(0), _iHeaderSize(headerLen),
		_iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	CPacket(int headerLen, int iBufferSize)
		: _iBufferSize(iBufferSize), _iPayloadSize(0), _iHeaderSize(headerLen),
		_iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen),
		_iHeaderReadPos(0), _iHeaderWritePos(0)
	{
		_chpBuffer = new char[_iBufferSize];
	}

	virtual	~CPacket()
	{
		delete[] _chpBuffer;
	}

	inline void Clear(void)
	{
		_iPayloadReadPos = _iHeaderSize;
		_iPayloadWritePos = _iHeaderSize;
		_iHeaderReadPos = 0;
		_iHeaderWritePos = 0;
	}

	inline int Resize(int iBufferSize)
	{
		if (iBufferSize > eBUFFER_MAX)
		{
			::printf("Requested Resize Size is too Big!! %d -> %d\n", _iBufferSize, iBufferSize);
			return -1;
		}

		char* chpNewBuffer = new char[iBufferSize];
		memcpy_s(chpNewBuffer, iBufferSize, _chpBuffer, _iBufferSize);
		delete[] _chpBuffer;

		_chpBuffer = chpNewBuffer;
		::printf("Resize!! %d -> %d\n", _iBufferSize, iBufferSize);
		_iBufferSize = iBufferSize;

		return _iBufferSize;
	}

	inline int	GetBufferSize(void) { return _iBufferSize; }

	inline bool IsPayloadEmpty(void) { return (_iPayloadWritePos == _iPayloadReadPos); }
	inline bool IsHeaderEmpty(void) { return (_iHeaderWritePos == _iHeaderReadPos); }
	inline bool IsEmpty(void) { return (IsPayloadEmpty() && IsHeaderEmpty()); }

	inline int	GetPayloadSize(void) { return _iPayloadWritePos - _iPayloadReadPos; }
	inline char* GetPayloadReadPtr(void) { return &_chpBuffer[_iPayloadReadPos]; }
	inline char* GetPayloadWritePtr(void) { return &_chpBuffer[_iPayloadWritePos]; }
	inline int	MovePayloadReadPos(int iSize)
	{
		if (iSize < 0)
		{
			::printf("Error! Func %s Line %d (req size - %d)\n",
				__func__, __LINE__, iSize);
			return -1;
		}

		if (_iPayloadReadPos + iSize > _iBufferSize)
		{
			::printf("Error! Func %s Line %d (read pos - %d, req size - %d)\n",
				__func__, __LINE__, _iPayloadReadPos, iSize);
			return -1;
		}

		_iPayloadReadPos += iSize;

		return iSize;
	}

	inline int	MovePayloadWritePos(int iSize)
	{
		if (iSize < 0)
		{
			::printf("Error! Func %s Line %d (req size - %d)\n",
				__func__, __LINE__, iSize);
			return -1;
		}

		if (_iPayloadWritePos + iSize > _iBufferSize)
		{
			::printf("Error! Func %s Line %d (write pos - %d, req size - %d)\n",
				__func__, __LINE__, _iPayloadWritePos, iSize);
			return -1;
		}

		_iPayloadWritePos += iSize;

		return iSize;
	}


	inline int GetHeaderSize(void) { return _iHeaderWritePos - _iHeaderReadPos; }
	inline char* GetHeaderReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }
	inline char* GetHeaderWritePtr(void) { return &_chpBuffer[_iHeaderWritePos]; }
	
	inline int	MoveHeaderReadPos(int iSize)
	{
		if (iSize < 0)
		{
			::printf("Error! Func %s Line %d (req size - %d)\n",
				__func__, __LINE__, iSize);
			return -1;
		}

		_iHeaderReadPos += iSize;

		if (_iHeaderReadPos > _iHeaderSize)
		{
			::printf("Error! Func %s Line %d (read pos - %d, req size - %d)\n",
				__func__, __LINE__, _iHeaderReadPos, iSize);
			return -1;
		}

		return iSize;
	}

	inline int	MoveHeaderWritePos(int iSize)
	{
		if (iSize < 0)
		{
			::printf("Error! Func %s Line %d (req size - %d)\n",
				__func__, __LINE__, iSize);
			return -1;
		}

		_iHeaderWritePos += iSize;

		if (_iHeaderWritePos > _iHeaderSize)
		{
			::printf("Error! Func %s Line %d (write pos - %d, req size - %d)\n",
				__func__, __LINE__, _iHeaderWritePos, iSize);
			return -1;
		}

		return iSize;
	}

	int GetPacketSize(void) { return _iPayloadWritePos - _iHeaderReadPos; }
	char* GetPacketReadPtr(void) { return &_chpBuffer[_iHeaderReadPos]; }

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

	inline CPacket& operator >> (float& fValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(float))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(float), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(double), __func__, __LINE__);
			return *this;
		}

		memcpy_s(&dValue, sizeof(double),
			&_chpBuffer[_iPayloadReadPos], sizeof(double));

		_iPayloadReadPos += sizeof(double);
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

	inline 	CPacket& operator >> (char& chValue)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < sizeof(char))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(char), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(BYTE), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(wchar_t), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(short), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(WORD), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(int), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(DWORD), __func__, __LINE__);
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
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iPayloadWritePos - _iPayloadReadPos, sizeof(__int64), __func__, __LINE__);
			return *this;
		}

		memcpy_s(&iValue, sizeof(__int64),
			&_chpBuffer[_iPayloadReadPos], sizeof(__int64));

		_iPayloadReadPos += sizeof(__int64);
		return *this;
	}

	inline int	GetPayloadData(char* chpDest, int iSize)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < iSize)
		{
			::printf("Used Size(%d) is small than Requested Size(%d)!\n",
				_iPayloadWritePos - _iPayloadReadPos, iSize);
			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iPayloadReadPos], iSize);

		_iPayloadReadPos += iSize;
		return iSize;
	}

	inline int	PeekPayloadData(char* chpDest, int iSize)
	{
		if (_iPayloadWritePos - _iPayloadReadPos < iSize)
		{
			::printf("Used Size(%d) is small than Requested Size(%d)!\n",
				_iPayloadWritePos - _iPayloadReadPos, iSize);
			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iPayloadReadPos], iSize);
		return iSize;
	}

	inline int	PutPayloadData(char* chpSrc, int iSrcSize)
	{
		if (_iBufferSize - _iPayloadWritePos < iSrcSize)
			Resize(_iBufferSize + (int)(iSrcSize * 1.5f));

		memcpy_s(&_chpBuffer[_iPayloadWritePos],
			_iBufferSize - _iPayloadWritePos,
			chpSrc, iSrcSize);

		_iPayloadWritePos += iSrcSize;
		return iSrcSize;
	}

	inline int	GetHeaderData(char* chpDest, int iSize)
	{
		if (_iHeaderSize < iSize)
		{
			::printf("Header(%d) is small than Requested Size(%d)! (read: %d)\n",
				_iHeaderSize, iSize, _iHeaderReadPos);
			return -1;
		}

		if (_iHeaderWritePos - _iHeaderReadPos < iSize)
		{
			::printf("Header(%d) is small than Requested Size(%d)! (read: %d)\n",
				_iHeaderSize, iSize, _iHeaderReadPos);
			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);

		_iHeaderReadPos += iSize;
		return iSize;
	}

	inline int	PeekHeaderData(char* chpDest, int iSize)
	{
		if (_iHeaderWritePos - _iHeaderReadPos < iSize)
		{
			::printf("Usable Size(%d) is small than Requested Size(%d)! (header read pos: %d)\n",
				_iHeaderWritePos - _iHeaderReadPos, iSize, _iHeaderReadPos);
			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);
		return iSize;
	}

	inline int PutHeaderData(char* chpSrc, int iSrcSize)
	{
		if (_iHeaderSize < _iHeaderWritePos + iSrcSize)
		{
			::printf("Usable size(%d) is small than Requested Size(%d)! (header write pos: %d)\n",
				_iHeaderSize - _iHeaderWritePos, iSrcSize, _iHeaderWritePos);
			return -1;
		}

		memcpy_s(&_chpBuffer[_iHeaderWritePos],
			_iHeaderSize, chpSrc, iSrcSize);

		_iHeaderWritePos += iSrcSize;

		return iSrcSize;
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
};

#endif
