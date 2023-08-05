#include "SerializePacket.h"

SerializePacket::SerializePacket()
    : _iBufferSize(eBUFFER_DEFAULT), _iDataSize(0), _iReadPos(0), _iWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

SerializePacket::SerializePacket(int iBufferSize)
    : _iBufferSize(iBufferSize), _iDataSize(0), _iReadPos(0), _iWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

SerializePacket::~SerializePacket()
{
    delete[] _chpBuffer;
}

void SerializePacket::Clear(void)
{
    _iReadPos = 0;
    _iWritePos = 0;
}

int SerializePacket::Resize(int iBufferSize)
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

int SerializePacket::MoveWritePos(int iSize)
{
    if (iSize < 0) return -1;
    _iWritePos += iSize;
    return iSize;
}

int SerializePacket::MoveReadPos(int iSize)
{
    if (iSize < 0) return -1;
    _iReadPos += iSize;
    return iSize;
}
