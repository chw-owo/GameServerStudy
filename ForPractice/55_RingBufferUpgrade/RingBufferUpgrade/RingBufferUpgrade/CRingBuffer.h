#pragma once
#include "Config.h"
#include "ErrorCode.h"
#include <stdio.h>
#include <string.h>

/*====================================================================

    <Ring Buffer>

    readPos는 비어있는 공간을,
    writePos는 마지막으로 넣은 공간을 가리킨다
    따라서 readPos == writePos는 버퍼가 비어있음을 의미하고
    버퍼가 찼을 때는 (readPos + 1)%_bufferSize == writePos 가 된다.

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

    inline int DirectEnqueueSize(void)
    {
        int writePos = _writePos;
        int readPos = _readPos;
        int bufferSize = _bufferSize;

        if (writePos >= readPos)
            return bufferSize - writePos - 1;
        else
            return readPos - writePos - 1;
    }

    inline int DirectDequeueSize(void)
    {
        int writePos = _writePos;
        int readPos = _readPos;
        int bufferSize = _bufferSize;

        if (writePos >= readPos)
            return writePos - readPos;
        else
            return bufferSize - readPos - 1;
    }

public:
    inline void ClearBuffer(void)
    {
        _readPos = 0;
        _writePos = 0;
        _useSize = 0;
        _bufferSize = _initBufferSize;
        _freeSize = _bufferSize - 1;
    }

    inline int Enqueue(char* chpData, int size)
    {
        int freeSize = _freeSize;
        int bufferSize = _bufferSize;
        int writePos = _writePos;
        int readPos = _readPos;

        if (size > dfRBUFFER_MAX_SIZE)
        {
            _errCode = ERR_ENQ_OVER;
            return ERR_RINGBUFFER;
        }

        else if (size > freeSize)
        {
            if (!Resize(bufferSize + (int)(size * 1.5f)))
            {
                _errCode = ERR_ENQ_OVER_AFTER_RESIZE;
                return ERR_RINGBUFFER;
            }
        }

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
        _writePos = (_writePos + size) % _bufferSize;

        return size;
    }

    inline int Dequeue(char* chpData, int size)
    {
        int useSize = _useSize;
        int bufferSize = _bufferSize;
        int writePos = _writePos;
        int readPos = _readPos;

        if (size > useSize)
        {
            _errCode = ERR_DEQ_OVER;
            return ERR_RINGBUFFER;
        }

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
        _readPos = (_readPos + size) % _bufferSize;

        return size;
    }

    inline  bool Resize(int size)
    {
        int useSize = _useSize;
        int bufferSize = _bufferSize;
        int writePos = _writePos;
        int readPos = _readPos;

        if (size > dfRBUFFER_MAX_SIZE)
        {
            _errCode = ERR_RESIZE_OVER_MAX;
            return false;
        }

        if (size < useSize)
        {
            _errCode = ERR_RESIZE_UNDER_USE;
            return false;
        }

        char* newBuffer = new char[size];
        memset(newBuffer, '\0', size);

        if (writePos > readPos)
        {
            memcpy_s(&newBuffer[1], size - 1, &_buffer[(readPos + 1) % bufferSize], useSize);
        }
        else if (writePos <= readPos)
        {
            int size1 = bufferSize - readPos - 1;
            int size2 = writePos + 1;
            memcpy_s(&newBuffer[1], size - 1, &_buffer[(readPos + 1) % bufferSize], size1);
            memcpy_s(&newBuffer[size1 + 1], size - (size1 + 1), &_buffer[0], size2);
        }

        delete[] _buffer;
        _buffer = newBuffer;
        _readPos = 0;
        _writePos = _useSize;
        _bufferSize = size;
        _freeSize = _bufferSize - _useSize - 1;

        return true;
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

