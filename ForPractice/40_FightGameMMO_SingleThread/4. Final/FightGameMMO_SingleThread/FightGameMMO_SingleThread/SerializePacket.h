#pragma once
#ifndef  __SERIALIZE_BUFFER__
#define  __SERIALIZE_BUFFER__

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include "SystemLog.h"

#define __SPACKET_DEBUG__

class CSerializePacket
{
public:

	enum en_BUFFER
	{
		// 패킷의 기본 버퍼 사이즈.
		eBUFFER_DEFAULT = 2048,
		eBUFFER_MAX = 4096
	};

	CSerializePacket()
		: _bufferSize(eBUFFER_DEFAULT), _dataSize(0), _readPos(0), _writePos(0)
	{
		_buffer = new char[_bufferSize];
	}

	CSerializePacket(int bufferSize)
		: _bufferSize(bufferSize), _dataSize(0), _readPos(0), _writePos(0)
	{
		_buffer = new char[_bufferSize];
	}

	~CSerializePacket()
	{
		delete[] _buffer;
	}

	inline void Clear(void)
	{
		_readPos = 0;
		_writePos = 0;
	}

	inline int Resize(int bufferSize)
	{
		if (bufferSize > eBUFFER_MAX)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: req %d max %d\n",
				_T(__FUNCTION__), __LINE__, bufferSize, eBUFFER_MAX);

			::wprintf(L"%s[%d]: buffer size %d, req size: %d\n",
				_T(__FUNCTION__), __LINE__, bufferSize, eBUFFER_MAX);
			return -1;
		}

		char* chpNewBuffer = new char[bufferSize];
		memcpy_s(chpNewBuffer, bufferSize, _buffer, _bufferSize);
		delete[] _buffer;

		_buffer = chpNewBuffer;
		_bufferSize = bufferSize;

		return _bufferSize;
	}

	inline int	MoveWritePos(int size)
	{
		if (size < 0) return -1;
		_writePos += size;
		return size;
	}

	inline int	MoveReadPos(int size)
	{
		if (size < 0) return -1;
		_readPos += size;
		return size;
	}

	inline char* GetWritePtr(void) { return &_buffer[_writePos]; }
	inline char* GetReadPtr(void) { return &_buffer[_readPos]; }

	inline int	GetBufferSize(void) { return _bufferSize; }
	inline int	GetDataSize(void) { return _writePos - _readPos; }
	inline bool IsEmpty(void) { return (_writePos == _readPos); }

	inline CSerializePacket& operator = (CSerializePacket& pSerializePacket)
	{
		*this = pSerializePacket;
		return *this;
	}

	inline CSerializePacket& operator << (float fValue)
	{
		if (_bufferSize - _writePos < sizeof(fValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&fValue, sizeof(fValue));

		_writePos += sizeof(fValue);
		return *this;
	}

	inline CSerializePacket& operator << (double dValue)
	{
		if (_bufferSize - _writePos < sizeof(dValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&dValue, sizeof(dValue));

		_writePos += sizeof(dValue);
		return *this;
	}

	inline CSerializePacket& operator >> (float& fValue)
	{
		if (_writePos - _readPos < sizeof(float))
		{
			

			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(float));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(float));

			return *this;
		}

		memcpy_s(&fValue, sizeof(float),
			&_buffer[_readPos], sizeof(float));

		_readPos += sizeof(float);
		return *this;
	}

	inline CSerializePacket& operator >> (double& dValue)
	{
		if (_writePos - _readPos < sizeof(double))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(double));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(double));

			return *this;
		}

		memcpy_s(&dValue, sizeof(double),
			&_buffer[_readPos], sizeof(double));

		_readPos += sizeof(double);
		return *this;
	}

	inline CSerializePacket& operator << (char chValue)
	{
		if (_bufferSize - _writePos < sizeof(chValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&chValue, sizeof(chValue));

		_writePos += sizeof(chValue);
		return *this;
	}

	inline CSerializePacket& operator << (unsigned char chValue)
	{
		if (_bufferSize - _writePos < sizeof(chValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&chValue, sizeof(chValue));

		_writePos += sizeof(chValue);
		return *this;
	}

	inline CSerializePacket& operator << (short shValue)
	{
		if (_bufferSize - _writePos < sizeof(shValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&shValue, sizeof(shValue));

		_writePos += sizeof(shValue);
		return *this;
	}
	inline CSerializePacket& operator << (unsigned short wValue)
	{
		if (_bufferSize - _writePos < sizeof(wValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&wValue, sizeof(wValue));

		_writePos += sizeof(wValue);
		return *this;
	}


	inline CSerializePacket& operator << (int iValue)
	{
		if (_bufferSize - _writePos < sizeof(iValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&iValue, sizeof(iValue));

		_writePos += sizeof(iValue);
		return *this;
	}

	inline CSerializePacket& operator << (long lValue)
	{
		if (_bufferSize - _writePos < sizeof(lValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&lValue, sizeof(lValue));

		_writePos += sizeof(lValue);
		return *this;
	}

	inline CSerializePacket& operator << (__int64 iValue)
	{
		if (_bufferSize - _writePos < sizeof(iValue))
			Resize((int)(_bufferSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			&iValue, sizeof(iValue));

		_writePos += sizeof(iValue);
		return *this;
	}


	inline CSerializePacket& operator >> (char& chValue)
	{
		if (_writePos - _readPos < sizeof(char))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(char));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(char));

			return *this;
		}

		memcpy_s(&chValue, sizeof(char),
			&_buffer[_readPos], sizeof(char));

		_readPos += sizeof(char);
		return *this;
	}

	inline CSerializePacket& operator >> (BYTE& byValue)
	{
		if (_writePos - _readPos < sizeof(BYTE))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(BYTE));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(BYTE));

			return *this;
		}

		memcpy_s(&byValue, sizeof(BYTE),
			&_buffer[_readPos], sizeof(BYTE));

		_readPos += sizeof(BYTE);
		return *this;
	}

	inline CSerializePacket& operator >> (wchar_t& szValue)
	{
		if (_writePos - _readPos < sizeof(wchar_t))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(wchar_t));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(wchar_t));

			return *this;
		}

		memcpy_s(&szValue, sizeof(wchar_t),
			&_buffer[_readPos], sizeof(wchar_t));

		_readPos += sizeof(wchar_t);
		return *this;
	}

	inline CSerializePacket& operator >> (short& shValue)
	{
		if (_writePos - _readPos < sizeof(short))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(short));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(short));

			return *this;
		}

		memcpy_s(&shValue, sizeof(short),
			&_buffer[_readPos], sizeof(short));

		_readPos += sizeof(short);
		return *this;
	}

	inline CSerializePacket& operator >> (WORD& wValue)
	{
		if (_writePos - _readPos < sizeof(WORD))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(WORD));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(WORD));

			return *this;
		}

		memcpy_s(&wValue, sizeof(WORD),
			&_buffer[_readPos], sizeof(WORD));

		_readPos += sizeof(WORD);
		return *this;
	}

	inline CSerializePacket& operator >> (int& iValue)
	{
		if (_writePos - _readPos < sizeof(int))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(int));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(int));

			return *this;
		}

		memcpy_s(&iValue, sizeof(int),
			&_buffer[_readPos], sizeof(int));

		_readPos += sizeof(int);
		return *this;
	}

	inline CSerializePacket& operator >> (DWORD& dwValue)
	{
		if (_writePos - _readPos < sizeof(DWORD))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(DWORD));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(DWORD));

			return *this;
		}

		memcpy_s(&dwValue, sizeof(DWORD),
			&_buffer[_readPos], sizeof(DWORD));

		_readPos += sizeof(DWORD);
		return *this;
	}

	inline CSerializePacket& operator >> (__int64& iValue)
	{
		if (_writePos - _readPos < sizeof(__int64))
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(__int64));

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, sizeof(__int64));

			return *this;
		}

		memcpy_s(&iValue, sizeof(__int64),
			&_buffer[_readPos], sizeof(__int64));

		_readPos += sizeof(__int64);
		return *this;
	}

	inline int	GetData(char* chpDest, int size)
	{
		if (_writePos - _readPos < size)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, size);

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, size);

			return -1;
		}

		memcpy_s(chpDest, size, &_buffer[_readPos], size);

		_readPos += size;
		return size;
	}

	inline int	PeekData(char* chpDest, int size)
	{
		if (_writePos - _readPos < size)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, size);

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, size);

			return -1;
		}

		memcpy_s(chpDest, size, &_buffer[_readPos], size);
		return size;
	}

	inline int CheckData(int size)
	{
		if (_writePos - _readPos < size)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, size);

			::wprintf(L"%s[%d]: used size %d < req size: %llu\n",
				_T(__FUNCTION__), __LINE__, _writePos - _readPos, size);

			return -1;
		}
	}

	inline int	PutData(char* src, int srcSize)
	{
		if (_bufferSize - _writePos < srcSize)
			Resize(_bufferSize + (int)(srcSize * 1.5f));

		memcpy_s(&_buffer[_writePos],
			_bufferSize - _writePos,
			src, srcSize);

		_writePos += srcSize;
		return srcSize;
	}

protected:
	int	_bufferSize;
	int	_dataSize;

private:
	char* _buffer;
	int _readPos;
	int _writePos;

};

#endif
