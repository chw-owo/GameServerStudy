#pragma once
#include "Config.h"
#include "ErrorCode.h"
#include <stdio.h>

/*====================================================================

    <Ring Buffer>

    readPos�� ����ִ� ������,
    writePos�� ���������� ���� ������ ����Ų��
    ���� readPos == writePos�� ���۰� ��������� �ǹ��ϰ�
    ���۰� á�� ���� (readPos + 1)%_bufferSize == writePos �� �ȴ�.

======================================================================*/

class CRingBuffer
{
public:
    inline CRingBuffer(void)
        : _initBufferSize(dfRBUFFER_DEF_SIZE), _bufferSize(dfRBUFFER_DEF_SIZE), _freeSize(dfRBUFFER_DEF_SIZE - 1)
    {
        _buffer = new char[_bufferSize];
    }

    inline CRingBuffer(int bufferSize)
        : _initBufferSize(bufferSize), _bufferSize(bufferSize), _freeSize(bufferSize - 1)
    {
        _buffer = new char[_bufferSize];
    }

    inline ~CRingBuffer(void)
    {
        delete[] _buffer;
    }

public:
    inline int GetBufferSize(void) { return _bufferSize; }
    inline int GetUseSize(void) { return _useSize; }
    inline int GetFreeSize(void) { return _freeSize; }
    inline char* GetReadPtr(void) { return  &_buffer[(_readPos + 1) % _bufferSize]; }
    inline char* GetWritePtr(void) { return &_buffer[(_writePos + 1) % _bufferSize]; }
    inline char* GetFrontPtr(void) { return &_buffer[0]; }

public:
    void ClearBuffer(void);
    int Peek(char* chpDest, int size);
    int Enqueue(char* chpData, int size);
    int Dequeue(char* chpData, int size);
    bool Resize(int size);

    int MoveReadPos(int size);
    int MoveWritePos(int size);
    int DirectEnqueueSize(void);
    int DirectDequeueSize(void);

public:
    inline void PrintBufferDataForDebug()
    {
        ::wprintf(L"\n");
        ::wprintf(L"Buffer Size: %d\n", _bufferSize);
        ::wprintf(L"Read: %d\n", _readPos);
        ::wprintf(L"Write: %d\n", _writePos);
        ::wprintf(L"Real Use Size: %d\n", _useSize);
        ::wprintf(L"Real Free Size: %d\n", _freeSize);
        ::wprintf(L"Direct Dequeue Size: %d\n", DirectDequeueSize());
        ::wprintf(L"Direct Enqueue Size: %d\n", DirectEnqueueSize());
        ::wprintf(L"\n");
    }

private:
    char* _buffer;
    int _readPos = 0;
    int _writePos = 0;

    int _initBufferSize;
    int _bufferSize;
    int _useSize = 0;
    int _freeSize = 0;

private:
    int _errCode;
};

