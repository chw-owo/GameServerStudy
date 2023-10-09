#pragma once
#include "SerializePacket.h"
#define BUF_MAX 8192

class PacketBuffer
{
public:
    int GetUseSize(void);
    int MoveReadPos(int iSize);
    int MoveWritePos(int iSize);
    SerializePacket* GetReadPosData(int idx);
    void SetWritePosData(SerializePacket* input);

private:
    SerializePacket* _buffer[BUF_MAX];
    int _readPos = 0;
    int _writePos = 0;

    int _bufferSize = BUF_MAX;
    int _useSize = 0;
    int _freeSize = BUF_MAX - 1;
};

