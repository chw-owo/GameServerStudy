#include "CPacket.h"

void CPacket::Clear(void)
{
	_iPayloadReadPos = _iHeaderSize;
	_iPayloadWritePos = _iHeaderSize;
	_iHeaderReadPos = 0;
	_iHeaderWritePos = 0;
	_usageCount = 0;
}

int CPacket::Resize(int iBufferSize)
{
	if (iBufferSize > dfRBUFFER_MAX_SIZE)
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

int CPacket::MovePayloadReadPos(int iSize)
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

	_iPayloadReadPos += iSize;
	return iSize;
}

int	CPacket::MovePayloadWritePos(int iSize)
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

	_iPayloadWritePos += iSize;

	return iSize;
}

int CPacket::MoveHeaderReadPos(int iSize)
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

int	CPacket::MoveHeaderWritePos(int iSize)
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
