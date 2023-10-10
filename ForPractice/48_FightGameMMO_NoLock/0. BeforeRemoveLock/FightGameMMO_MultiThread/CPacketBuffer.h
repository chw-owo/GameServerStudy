#pragma once
#include "CPacket.h"
#define BUF_MAX 8192

class CPacketBuffer
{
public:
    int GetUseSize(void);
    int MoveReadPos(int iSize);
    int MoveWritePos(int iSize);
    CPacket* GetReadPosData(int idx);
    void SetWritePosData(CPacket* input);

private:
    CPacket* _buffer[BUF_MAX];
    int _readPos = 0;
    int _writePos = 0;

    int _bufferSize = BUF_MAX;
    int _useSize = 0;
    int _freeSize = BUF_MAX - 1;
};

