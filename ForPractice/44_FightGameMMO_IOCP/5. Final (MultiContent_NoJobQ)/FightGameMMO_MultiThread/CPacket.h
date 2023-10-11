#pragma once
#ifndef  __SERIALIZE_BUFFER__
#define  __SERIALIZE_BUFFER__

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "Protocol.h"
#include "CSystemLog.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] req %d, max %d\n",
				_T(__FUNCTION__), __LINE__, iBufferSize, eBUFFER_MAX);
			::wprintf(L"%s[%d] req %d, max %d\n",
				_T(__FUNCTION__), __LINE__, iBufferSize, eBUFFER_MAX);

			return -1;
		}

		char* chpNewBuffer = new char[iBufferSize];
		memcpy_s(chpNewBuffer, iBufferSize, _chpBuffer, _iBufferSize);
		delete[] _chpBuffer;

		_chpBuffer = chpNewBuffer;
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);
			::wprintf(L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);

			return -1;
		}

		if (_iPayloadReadPos + iSize > _iBufferSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] read pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iPayloadReadPos + iSize, _iBufferSize);
			::wprintf(L"%s[%d] read pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iPayloadReadPos + iSize, _iBufferSize);

			return -1;
		}

		_iPayloadReadPos += iSize;

		return iSize;
	}

	inline int	MovePayloadWritePos(int iSize)
	{
		if (iSize < 0)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);
			::wprintf(L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);

			return -1;
		}

		if (_iPayloadWritePos + iSize > _iBufferSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] write pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iPayloadReadPos + iSize, _iBufferSize);
			::wprintf(L"%s[%d] write pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iPayloadReadPos + iSize, _iBufferSize);

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);
			::wprintf(L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);

			return -1;
		}

		if (_iHeaderReadPos + iSize > _iHeaderSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] read pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iHeaderReadPos + iSize, _iHeaderSize);
			::wprintf(L"%s[%d] read pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iHeaderReadPos + iSize, _iHeaderSize);

			return -1;
		}

		_iHeaderReadPos += iSize;

		return iSize;
	}

	inline int	MoveHeaderWritePos(int iSize)
	{
		if (iSize < 0)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);
			::wprintf(L"%s[%d] req %d < 0\n",
				_T(__FUNCTION__), __LINE__, iSize);

			return -1;
		}

		if (_iHeaderWritePos + iSize > _iHeaderSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] write pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iHeaderWritePos + iSize, _iHeaderSize);
			::wprintf(L"%s[%d] write pos + req %d, buffer size %d\n",
				_T(__FUNCTION__), __LINE__, _iHeaderWritePos + iSize, _iHeaderSize);

			return -1;
		}

		_iHeaderWritePos += iSize;

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(float));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(float));

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__, 
				_iPayloadWritePos - _iPayloadReadPos, sizeof(double));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(double));

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(char));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(char));

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(BYTE));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(BYTE));

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(wchar_t));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(wchar_t));
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(short));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(short));
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(WORD));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(WORD));
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(int));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(int));
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(DWORD));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(DWORD));
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(__int64));

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, sizeof(__int64));
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, iSize);

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
				_iPayloadWritePos - _iPayloadReadPos, iSize);

			::wprintf(L"%s[%d]: used size %d, req size %lld\n", _T(__FUNCTION__), __LINE__,
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: header max size %d, req size %lld\n", 
				_T(__FUNCTION__), __LINE__, _iHeaderSize, iSize);

			::wprintf(L"%s[%d]: header max size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderSize, iSize);
			return -1;
		}

		if (_iHeaderWritePos - _iHeaderReadPos < iSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: header use size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderWritePos - _iHeaderReadPos, iSize);

			::wprintf(L"%s[%d]: header use size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderWritePos - _iHeaderReadPos, iSize);
			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);

		_iHeaderReadPos += iSize;
		return iSize;
	}

	inline int	PeekHeaderData(char* chpDest, int iSize)
	{
		if (_iHeaderSize < iSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: header max size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderSize, iSize);

			::wprintf(L"%s[%d]: header max size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderSize, iSize);
			return -1;
		}

		if (_iHeaderWritePos - _iHeaderReadPos < iSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: header use size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderWritePos - _iHeaderReadPos, iSize);

			::wprintf(L"%s[%d]: header use size %d, req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderWritePos - _iHeaderReadPos, iSize);
			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);
		return iSize;
	}

	inline int PutHeaderData(char* chpSrc, int iSrcSize)
	{
		if (_iHeaderSize < _iHeaderWritePos + iSrcSize)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: header max size %d, write pos + req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderSize, _iHeaderWritePos + iSrcSize);

			::wprintf(L"%s[%d]: header max size %d, write pos + req size %lld\n",
				_T(__FUNCTION__), __LINE__, _iHeaderSize, _iHeaderWritePos + iSrcSize);
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
