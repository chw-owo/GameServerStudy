#pragma once
#include "Config.h"

#ifdef LANSERVER
#include "CLanRecvPacket.h"
#include "CTlsPool.h"

class CLanMsg
{
	friend CTlsPool<CLanMsg>;

private:
	inline CLanMsg() { }
	inline CLanMsg(CLanRecvPacket* packet, unsigned short size) 
		: _packet(packet), _size(size), _startPos(packet->GetPayloadReadPos()) { }

public: // TO-DO: private
	int _startPos = 0;
	unsigned short _size = 0;
	CLanRecvPacket* _packet = nullptr;

public:
	static CTlsPool<CLanMsg> _pool;

	inline static CLanMsg* Alloc(CLanRecvPacket* packet, short size)
	{
		CLanMsg* recvMsg = _pool.Alloc(packet, size);
		packet->_msgs->Push(recvMsg);
		return recvMsg;
	}

	inline static void Free(CLanMsg* packet)
	{
		CLanRecvPacket::Free(packet->_packet);
	}

	inline static int GetNodeCount()
	{
		return _pool.GetNodeCount();
	}

public:
	inline CLanMsg& operator = (CLanMsg& clSrCLanRecvPacket)
	{
		*this = clSrCLanRecvPacket;
		return *this;
	}

	inline CLanMsg& operator >> (float& fValue)
	{
		if (_size < sizeof(float))
		{
			_packet->_errCode = ERR_GET_FLOAT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&fValue, sizeof(float),
			&_packet->_chpBuffer[_startPos], sizeof(float));

		_startPos += sizeof(float);
		return *this;
	}
	inline CLanMsg& operator >> (double& dValue)
	{
		if (_size < sizeof(double))
		{
			_packet->_errCode = ERR_GET_DOUBLE_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&dValue, sizeof(double),
			&_packet->_chpBuffer[_startPos], sizeof(double));

		_startPos += sizeof(double);
		return *this;
	}
	inline CLanMsg& operator >> (char& chValue)
	{
		if (_size < sizeof(char))
		{
			_packet->_errCode = ERR_GET_CHAR_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&chValue, sizeof(char),
			&_packet->_chpBuffer[_startPos], sizeof(char));

		_startPos += sizeof(char);
		return *this;
	}
	inline CLanMsg& operator >> (BYTE& byValue)
	{
		if (_size < sizeof(BYTE))
		{
			_packet->_errCode = ERR_GET_BYTE_OVER;

			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&byValue, sizeof(BYTE),
			&_packet->_chpBuffer[_startPos], sizeof(BYTE));

		_startPos += sizeof(BYTE);
		return *this;
	}
	inline CLanMsg& operator >> (wchar_t& szValue)
	{
		if (_size < sizeof(wchar_t))
		{
			_packet->_errCode = ERR_GET_WCHAR_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&szValue, sizeof(wchar_t),
			&_packet->_chpBuffer[_startPos], sizeof(wchar_t));

		_startPos += sizeof(wchar_t);
		return *this;
	}
	inline CLanMsg& operator >> (short& shValue)
	{
		if (_size < sizeof(short))
		{
			_packet->_errCode = ERR_GET_SHORT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&shValue, sizeof(short),
			&_packet->_chpBuffer[_startPos], sizeof(short));

		_startPos += sizeof(short);
		return *this;
	}
	inline CLanMsg& operator >> (WORD& wValue)
	{
		if (_size < sizeof(WORD))
		{
			_packet->_errCode = ERR_GET_WORD_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&wValue, sizeof(WORD),
			&_packet->_chpBuffer[_startPos], sizeof(WORD));

		_startPos += sizeof(WORD);
		return *this;
	}
	inline CLanMsg& operator >> (int& iValue)
	{
		if (_size < sizeof(int))
		{
			_packet->_errCode = ERR_GET_INT_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&iValue, sizeof(int),
			&_packet->_chpBuffer[_startPos], sizeof(int));

		_startPos += sizeof(int);
		return *this;
	}
	inline CLanMsg& operator >> (DWORD& dwValue)
	{
		if (_size < sizeof(DWORD))
		{
			_packet->_errCode = ERR_GET_DWORD_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&dwValue, sizeof(DWORD),
			&_packet->_chpBuffer[_startPos], sizeof(DWORD));

		_startPos += sizeof(DWORD);
		return *this;
	}
	inline CLanMsg& operator >> (__int64& iValue)
	{
		if (_size < sizeof(__int64))
		{
			_packet->_errCode = ERR_GET_INT64_OVER;
			throw (int)ERR_PACKET;
			return *this;
		}

		memcpy_s(&iValue, sizeof(__int64),
			&_packet->_chpBuffer[_startPos], sizeof(__int64));

		_startPos += sizeof(__int64);
		return *this;
	}

	inline int GetPayloadData(char* chpDest, int iSize)
	{
		if (_size < iSize)
		{
			_packet->_errCode = ERR_GET_PAYLOAD_OVER;
			return ERR_PACKET;
		}

		memcpy_s(chpDest, iSize, &_packet->_chpBuffer[_startPos], iSize);

		_startPos += iSize;
		return iSize;
	}
};

#endif