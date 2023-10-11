#include "CPacket.h"

void CPacket::Clear(void)
{
	_iPayloadReadPos = _iHeaderSize;
	_iPayloadWritePos = _iHeaderSize;
	_iHeaderReadPos = 0;
	_iHeaderWritePos = 0;
}

int CPacket::Resize(int iBufferSize)
{
	if (iBufferSize > dfRBUFFER_MAX_SIZE)
	{
		// TO-DO: 외부로 전달
		return -1;
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
		// TO-DO: 외부로 전달
		return -1;
	}

	if (_iPayloadReadPos + iSize > _iBufferSize)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	_iPayloadReadPos += iSize;
	return iSize;
}

int	CPacket::MovePayloadWritePos(int iSize)
{
	if (iSize < 0)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	if (_iPayloadWritePos + iSize > _iBufferSize)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	_iPayloadWritePos += iSize;

	return iSize;
}

int CPacket::MoveHeaderReadPos(int iSize)
{
	if (iSize < 0)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	if (_iHeaderReadPos + iSize > _iHeaderSize)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	_iHeaderReadPos += iSize;

	return iSize;
}

int	CPacket::MoveHeaderWritePos(int iSize)
{
	if (iSize < 0)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	if (_iHeaderWritePos + iSize > _iHeaderSize)
	{
		// TO-DO: 외부로 전달
		return -1;
	}

	_iHeaderWritePos += iSize;

	return iSize;
}
