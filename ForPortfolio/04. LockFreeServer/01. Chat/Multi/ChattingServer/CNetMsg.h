#pragma once
#include "Config.h"

#ifdef NETSERVER
#include "CNetRecvPacket.h"
#include "CTlsPool.h"

class CNetMsg
{
	friend CTlsPool<CNetMsg>;

private:
	inline CNetMsg() { }
	inline CNetMsg(CNetRecvPacket* packet) : _packet(packet),
		_readPos(packet->GetPayloadReadPos()), _writePos(packet->GetPayloadWritePos()) { }

public: // TO-DO: private
	int _readPos = 0;
	int _writePos = 0;
	CNetRecvPacket* _packet = nullptr;

public:
	static CTlsPool<CNetMsg> _pool;

	inline static CNetMsg* Alloc(CNetRecvPacket* packet)
	{
		CNetMsg* recvNetPacket = _pool.Alloc(packet);
		packet->_msgs->Push(recvNetPacket);
		return recvNetPacket;
	}

	inline static void Free(CNetMsg* packet)
	{
		CNetRecvPacket::Free(packet->_packet);
	}

	inline static int GetNodeCount()
	{
		return _pool.GetNodeCount();
	}

public:
	inline CNetMsg& operator = (CNetMsg& clSrCNetRecvPacket)
	{
		*this = clSrCNetRecvPacket;
		return *this;
	}

	inline CNetMsg& operator >> (float& fValue)
	{
		if (_writePos - _readPos < sizeof(float))
		{
			_packet->_errCode = ERR_GET_FLOAT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&fValue, sizeof(float),
			&_packet->_chpBuffer[_readPos], sizeof(float));

		_readPos += sizeof(float);
		return *this;
	}
	inline CNetMsg& operator >> (double& dValue)
	{
		if (_writePos - _readPos < sizeof(double))
		{
			_packet->_errCode = ERR_GET_DOUBLE_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&dValue, sizeof(double),
			&_packet->_chpBuffer[_readPos], sizeof(double));

		_readPos += sizeof(double);
		return *this;
	}
	inline CNetMsg& operator >> (char& chValue)
	{
		if (_writePos - _readPos < sizeof(char))
		{
			_packet->_errCode = ERR_GET_CHAR_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&chValue, sizeof(char),
			&_packet->_chpBuffer[_readPos], sizeof(char));

		_readPos += sizeof(char);
		return *this;
	}
	inline CNetMsg& operator >> (BYTE& byValue)
	{
		if (_writePos - _readPos < sizeof(BYTE))
		{
			_packet->_errCode = ERR_GET_BYTE_OVER;

			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&byValue, sizeof(BYTE),
			&_packet->_chpBuffer[_readPos], sizeof(BYTE));

		_readPos += sizeof(BYTE);
		return *this;
	}
	inline CNetMsg& operator >> (wchar_t& szValue)
	{
		if (_writePos - _readPos < sizeof(wchar_t))
		{
			_packet->_errCode = ERR_GET_WCHAR_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&szValue, sizeof(wchar_t),
			&_packet->_chpBuffer[_readPos], sizeof(wchar_t));

		_readPos += sizeof(wchar_t);
		return *this;
	}
	inline CNetMsg& operator >> (short& shValue)
	{
		if (_writePos - _readPos < sizeof(short))
		{
			_packet->_errCode = ERR_GET_SHORT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&shValue, sizeof(short),
			&_packet->_chpBuffer[_readPos], sizeof(short));

		_readPos += sizeof(short);
		return *this;
	}
	inline CNetMsg& operator >> (WORD& wValue)
	{
		if (_writePos - _readPos < sizeof(WORD))
		{
			_packet->_errCode = ERR_GET_WORD_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&wValue, sizeof(WORD),
			&_packet->_chpBuffer[_readPos], sizeof(WORD));

		_readPos += sizeof(WORD);
		return *this;
	}
	inline CNetMsg& operator >> (int& iValue)
	{
		if (_writePos - _readPos < sizeof(int))
		{
			_packet->_errCode = ERR_GET_INT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&iValue, sizeof(int),
			&_packet->_chpBuffer[_readPos], sizeof(int));

		_readPos += sizeof(int);
		return *this;
	}
	inline CNetMsg& operator >> (DWORD& dwValue)
	{
		if (_writePos - _readPos < sizeof(DWORD))
		{
			_packet->_errCode = ERR_GET_DWORD_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&dwValue, sizeof(DWORD),
			&_packet->_chpBuffer[_readPos], sizeof(DWORD));

		_readPos += sizeof(DWORD);
		return *this;
	}
	inline CNetMsg& operator >> (__int64& iValue)
	{
		if (_writePos - _readPos < sizeof(__int64))
		{
			_packet->_errCode = ERR_GET_INT64_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&iValue, sizeof(__int64),
			&_packet->_chpBuffer[_readPos], sizeof(__int64));

		_readPos += sizeof(__int64);
		return *this;
	}

	inline int GetPayloadData(char* chpDest, int iSize)
	{
		if (_writePos - _readPos < iSize)
		{
			_packet->_errCode = ERR_GET_PAYLOAD_OVER;
			return ERR_PACKET;
		}

		memcpy_s(chpDest, iSize, &_packet->_chpBuffer[_readPos], iSize);

		_readPos += iSize;
		return iSize;
	}
};

#endif