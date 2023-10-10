#include "PacketBuffer.h"
#include <stdio.h>

int PacketBuffer::GetUseSize(void)
{
    return _useSize;
}

int PacketBuffer::MoveReadPos(int iSize)
{
    if (iSize > _useSize)
    {
        ::printf("Error! Func %s Line %d: used %d\n",
            __func__, __LINE__, _useSize);
        return -1;
    }

    _readPos = (_readPos + iSize) % _bufferSize;
    _useSize -= iSize;
    _freeSize += iSize;

    return iSize;
}

int PacketBuffer::MoveWritePos(int iSize)
{
    if (iSize > _freeSize)
    {
        ::printf("Error! Func %s Line %d: free %d\n", 
            __func__, __LINE__, _freeSize);
        return -1;
    }

    _writePos = (_writePos + iSize) % _bufferSize;
    _useSize += iSize;
    _freeSize -= iSize;

    return iSize;
}

SerializePacket* PacketBuffer::GetReadPosData(int idx)
{
    int readPos = (_readPos + idx + 1) % _bufferSize;
	return _buffer[readPos];
}

void PacketBuffer::SetWritePosData(SerializePacket* input)
{
    int writePos = (_writePos + 1)% _bufferSize;
    _buffer[writePos] = input;
}


