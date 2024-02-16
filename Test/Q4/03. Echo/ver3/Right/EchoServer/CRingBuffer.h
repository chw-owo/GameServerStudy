#pragma once
#include "Config.h"
#include "ErrorCode.h"
#include <stdio.h>
#include <string.h>

class CRingBuffer
{
public:
    inline CRingBuffer(void)
        : _bufferSize(dfRBUFFER_DEF_SIZE), _freeSize(dfRBUFFER_DEF_SIZE - 1)
    {
        _buffer = new char[_bufferSize];
    }

    inline CRingBuffer(int bufferSize)
        : _bufferSize(bufferSize), _freeSize(bufferSize - 1)
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

public:
    void ClearBuffer(void)
    {
        _readPos = 0;
        _writePos = 0;
        _useSize = 0;
        _freeSize = _bufferSize - 1;
    }

    int Enqueue(char* chpData, int size)
    {
        if (size > _freeSize)
        {
            _errCode = ERR_ENQ_OVER;
            return ERR_RINGBUFFER;
        }

        int readPos = _readPos;
        int writePos = _writePos;
        int bufferSize = _bufferSize;

        int directEnqueueSize;
        if (writePos >= readPos) 
            directEnqueueSize = bufferSize - writePos - 1;
        else 
            directEnqueueSize = readPos - writePos - 1;

        if (size <= directEnqueueSize)
        {
            memcpy_s(&_buffer[(writePos + 1) % bufferSize], size, chpData, size);
        }
        else
        {
            int size1 = directEnqueueSize;
            int size2 = size - size1;
            memcpy_s(&_buffer[(writePos + 1) % bufferSize], size1, chpData, size1);
            memcpy_s(_buffer, size2, &chpData[size1], size2);
        }

        _useSize += size;
        _freeSize -= size;
        _writePos = (writePos + size) % bufferSize;

        return size;
    }

    int Dequeue(char* chpData, int size)
    {
        if (size > _useSize)
        {
            _errCode = ERR_DEQ_OVER;
            return ERR_RINGBUFFER;
        }

        int readPos = _readPos;
        int writePos = _writePos;
        int bufferSize = _bufferSize;

        int directDequeueSize;
        if (writePos >= readPos)
            directDequeueSize = writePos - readPos;
        else
            directDequeueSize = bufferSize - readPos - 1;

        if (size <= directDequeueSize)
        {
            memcpy_s(chpData, size, &_buffer[(readPos + 1) % bufferSize], size);
        }
        else
        {
            int size1 = directDequeueSize;
            int size2 = size - size1;
            memcpy_s(chpData, size1, &_buffer[(readPos + 1) % bufferSize], size1);
            memcpy_s(&chpData[size1], size2, _buffer, size2);
        }

        _useSize -= size;
        _freeSize += size;
        _readPos = (readPos + size) % bufferSize;

        return size;
    }

private:
    char* _buffer;
    int _readPos = 0;
    int _writePos = 0;
    int _bufferSize;
    int _useSize = 0;
    int _freeSize = 0;

private:
    int _errCode;
};
