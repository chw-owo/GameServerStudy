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
	~SerializePacket();

	void Clear(void);
	int Resize(int iBufferSize);

	int	MoveWritePos(int iSize);
	int	MoveReadPos(int iSize);
	inline char* GetWritePtr(void) { return &_chpBuffer[_iWritePos]; }
	inline char* GetReadPtr(void) { return &_chpBuffer[_iReadPos]; }

	inline int	GetBufferSize(void) { return _iBufferSize; }
	inline int	GetDataSize(void) { return _iWritePos - _iReadPos; }
	inline bool IsEmpty(void) { return (_iWritePos == _iReadPos); }

	inline SerializePacket& operator = (SerializePacket& clSrSerializePacket)
	{
		*this = clSrSerializePacket;
		return *this;
	}

	inline SerializePacket& operator << (float fValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(fValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&fValue, sizeof(fValue));

		_iWritePos += sizeof(fValue);
		return *this;
	}

	inline SerializePacket& operator << (double dValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(dValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&dValue, sizeof(dValue));

		_iWritePos += sizeof(dValue);
		return *this;
	}

	inline SerializePacket& operator >> (float& fValue)
	{
		if (_iWritePos - _iReadPos < sizeof(float))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(float), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(float));

			return *this;
		}

		memcpy_s(&fValue, sizeof(float),
			&_chpBuffer[_iReadPos], sizeof(float));

		_iReadPos += sizeof(float);
		return *this;
	}

	inline SerializePacket& operator >> (double& dValue)
	{
		if (_iWritePos - _iReadPos < sizeof(double))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(double), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(double));
			return *this;
		}

		memcpy_s(&dValue, sizeof(double),
			&_chpBuffer[_iReadPos], sizeof(double));

		_iReadPos += sizeof(double);
		return *this;
	}

	inline SerializePacket& operator << (char chValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(chValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&chValue, sizeof(chValue));

		_iWritePos += sizeof(chValue);
		return *this;
	}

	inline SerializePacket& operator << (unsigned char chValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(chValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&chValue, sizeof(chValue));

		_iWritePos += sizeof(chValue);
		return *this;
	}

	inline SerializePacket& operator << (short shValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(shValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&shValue, sizeof(shValue));

		_iWritePos += sizeof(shValue);
		return *this;
	}
	inline SerializePacket& operator << (unsigned short wValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(wValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&wValue, sizeof(wValue));

		_iWritePos += sizeof(wValue);
		return *this;
	}


	inline SerializePacket& operator << (int iValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(iValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&iValue, sizeof(iValue));

		_iWritePos += sizeof(iValue);
		return *this;
	}

	inline SerializePacket& operator << (long lValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(lValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&lValue, sizeof(lValue));

		_iWritePos += sizeof(lValue);
		return *this;
	}

	inline SerializePacket& operator << (__int64 iValue)
	{
		if (_iBufferSize - _iWritePos < sizeof(iValue))
			Resize((int)(_iBufferSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			&iValue, sizeof(iValue));

		_iWritePos += sizeof(iValue);
		return *this;
	}


	inline SerializePacket& operator >> (char& chValue)
	{
		if (_iWritePos - _iReadPos < sizeof(char))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(char), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(char));
			return *this;
		}

		memcpy_s(&chValue, sizeof(char),
			&_chpBuffer[_iReadPos], sizeof(char));

		_iReadPos += sizeof(char);
		return *this;
	}

	inline SerializePacket& operator >> (BYTE& byValue)
	{
		if (_iWritePos - _iReadPos < sizeof(BYTE))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!\n",
				_iWritePos - _iReadPos, sizeof(BYTE));

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(BYTE));
			return *this;
		}

		memcpy_s(&byValue, sizeof(BYTE),
			&_chpBuffer[_iReadPos], sizeof(BYTE));

		_iReadPos += sizeof(BYTE);
		return *this;
	}

	inline SerializePacket& operator >> (wchar_t& szValue)
	{
		if (_iWritePos - _iReadPos < sizeof(wchar_t))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(wchar_t), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(wchar_t));

			return *this;
		}

		memcpy_s(&szValue, sizeof(wchar_t),
			&_chpBuffer[_iReadPos], sizeof(wchar_t));

		_iReadPos += sizeof(wchar_t);
		return *this;
	}

	inline SerializePacket& operator >> (short& shValue)
	{
		if (_iWritePos - _iReadPos < sizeof(short))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(short), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(short));
			return *this;
		}

		memcpy_s(&shValue, sizeof(short),
			&_chpBuffer[_iReadPos], sizeof(short));

		_iReadPos += sizeof(short);
		return *this;
	}

	inline SerializePacket& operator >> (WORD& wValue)
	{
		if (_iWritePos - _iReadPos < sizeof(WORD))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(WORD), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(WORD));
			return *this;
		}

		memcpy_s(&wValue, sizeof(WORD),
			&_chpBuffer[_iReadPos], sizeof(WORD));

		_iReadPos += sizeof(WORD);
		return *this;
	}

	inline SerializePacket& operator >> (int& iValue)
	{
		if (_iWritePos - _iReadPos < sizeof(int))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(int), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(int));
			return *this;
		}

		memcpy_s(&iValue, sizeof(int),
			&_chpBuffer[_iReadPos], sizeof(int));

		_iReadPos += sizeof(int);
		return *this;
	}

	inline SerializePacket& operator >> (DWORD& dwValue)
	{
		if (_iWritePos - _iReadPos < sizeof(DWORD))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(DWORD), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(DWORD));

			return *this;
		}

		memcpy_s(&dwValue, sizeof(DWORD),
			&_chpBuffer[_iReadPos], sizeof(DWORD));

		_iReadPos += sizeof(DWORD);
		return *this;
	}

	inline SerializePacket& operator >> (__int64& iValue)
	{
		if (_iWritePos - _iReadPos < sizeof(__int64))
		{
			::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
				_iWritePos - _iReadPos, sizeof(__int64), __func__, __LINE__);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, sizeof(__int64));

			return *this;
		}

		memcpy_s(&iValue, sizeof(__int64),
			&_chpBuffer[_iReadPos], sizeof(__int64));

		_iReadPos += sizeof(__int64);
		return *this;
	}

	inline int	GetData(char* chpDest, int iSize)
	{
		if (_iWritePos - _iReadPos < iSize)
		{
			::printf("Used Size(%d) is small than Requested Size(%d)!\n",
				_iWritePos - _iReadPos, iSize);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, iSize);

			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iReadPos], iSize);

		_iReadPos += iSize;
		return iSize;
	}

	inline int	PeekData(char* chpDest, int iSize)
	{
		if (_iWritePos - _iReadPos < iSize)
		{
			::printf("Used Size(%d) is small than Requested Size(%d)!\n",
				_iWritePos - _iReadPos, iSize);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, iSize);

			return -1;
		}

		memcpy_s(chpDest, iSize, &_chpBuffer[_iReadPos], iSize);
		return iSize;
	}

	inline int  CheckData(int iSize)
	{
		if (_iWritePos - _iReadPos < iSize)
		{
			::printf("Used Size(%d) is small than Requested Size(%d)!\n",
				_iWritePos - _iReadPos, iSize);

			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: used size %d < req size: %d\n",
				_T(__FUNCTION__), __LINE__, _iWritePos - _iReadPos, iSize);

			return -1;
		}
	}

	inline int	PutData(char* chpSrc, int iSrcSize)
	{
		if (_iBufferSize - _iWritePos < iSrcSize)
			Resize(_iBufferSize + (int)(iSrcSize * 1.5f));

		memcpy_s(&_chpBuffer[_iWritePos],
			_iBufferSize - _iWritePos,
			chpSrc, iSrcSize);

		_iWritePos += iSrcSize;
		return iSrcSize;
	}



protected:
	int	_iBufferSize;
	int	_iDataSize;

private:
	char* _chpBuffer;
	int _iReadPos;
	int _iWritePos;

};

#endif
