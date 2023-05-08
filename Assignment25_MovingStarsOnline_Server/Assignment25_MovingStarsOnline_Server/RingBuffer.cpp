#include "RingBuffer.h"
#include <iostream>
//#define C_DEBUG

RingBuffer::RingBuffer(void) : _bufferSize(DEFAULT_BUF_SIZE), _freeSize(DEFAULT_BUF_SIZE)
{
    _buffer = new char[_bufferSize]();
}

RingBuffer::RingBuffer(int bufferSize) : _bufferSize(bufferSize), _freeSize(bufferSize)
{
    _buffer = new char[_bufferSize]();
}

RingBuffer::~RingBuffer(void)
{
    delete[] _buffer;
}

int RingBuffer::GetBufferSize(void)
{
    return _bufferSize;
}

int RingBuffer::GetUseSize(void)
{
#ifdef C_DEBUG
    int useSize = 0;

    if (_writePos > _readPos)
    {
        useSize = _writePos - _readPos;
    }
    else if (_writePos < _readPos)
    {
        useSize = _bufferSize - _readPos + _writePos;
    }

    if (useSize != _useSize)
        return -1;
#endif
    return _useSize;
}

int RingBuffer::GetFreeSize(void)
{
#ifdef C_DEBUG
    int freeSize = _bufferSize;

    if (_writePos > _readPos)
    {
        freeSize = _bufferSize - _writePos + _readPos;
    }
    else if (_writePos < _readPos)
    {
        freeSize = _readPos - _writePos;
    }

    if (freeSize != _freeSize)
        return -1;
#endif
    return _freeSize;
}

int RingBuffer::DirectEnqueueSize(void)
{
    int directEnqueueSize = -1;

    if (_writePos >= _readPos)
    {
        directEnqueueSize = _bufferSize - _writePos;
    }
    else
    {
        directEnqueueSize = GetFreeSize();
    }

    return directEnqueueSize;
}

int RingBuffer::DirectDequeueSize(void)
{
    int directDequeueSize = -1;

    if (_writePos >= _readPos)
    {
        directDequeueSize = GetUseSize();
    }
    else
    {
        directDequeueSize = _bufferSize - _readPos;
    }

    return directDequeueSize;
}


int RingBuffer::Enqueue(char* chpData, int iSize)
{
#ifdef C_DEBUG
    if (GetFreeSize() < 0 || DirectEnqueueSize() < 0)
    {
        printf("Enqueue Error!\n");
        return -1;
    }
#endif

    if (iSize > GetFreeSize())
    {
        if (!Resize(_bufferSize + iSize))
            return -1;
    }

    if (iSize <= DirectEnqueueSize())
    {
        memcpy_s(&_buffer[_writePos], DirectEnqueueSize(), chpData, iSize);
    }
    else
    {
#ifdef C_DEBUG
        if (_writePos < _readPos ||
            ((_writePos > _readPos) &&
                ((_writePos + iSize) % _bufferSize >= _readPos)))
            return -1;
#endif
        int size1 = DirectEnqueueSize();
        int size2 = iSize - size1;
        memcpy_s(&_buffer[_writePos], size1, chpData, size1);
        memcpy_s(_buffer, size2, &chpData[size1], size2);
    }

    _useSize += iSize;
    _freeSize -= iSize;
    _writePos = (_writePos + iSize) % _bufferSize;

#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return -1;
#endif

    return iSize;
}

int RingBuffer::Dequeue(char* chpData, int iSize)
{
#ifdef C_DEBUG
    if (GetUseSize() < 0 || DirectDequeueSize() < 0)
    {
        printf("Dequeue Error!\n");
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        printf("Dequeuing size is too Big\n");
        return -1;
    }

    if (iSize <= DirectDequeueSize())
    {
        memcpy_s(chpData, iSize, &_buffer[_readPos], iSize);
    }
    else
    {
#ifdef C_DEBUG
        if (_writePos > _readPos ||
            ((_writePos < _readPos) &&
                ((_readPos + iSize) % _bufferSize > _writePos)))
            return -1;
#endif
        int size1 = DirectDequeueSize();
        int size2 = iSize - size1;
        memcpy_s(chpData, size1, &_buffer[_readPos], size1);
        memcpy_s(&chpData[size1], size2, _buffer, size2);
    }

    _useSize -= iSize;
    _freeSize += iSize;
    _readPos = (_readPos + iSize) % _bufferSize;

#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return -1;
#endif
    return iSize;
}

int RingBuffer::Peek(char* chpDest, int iSize)
{
#ifdef C_DEBUG
    if (GetUseSize() < 0 || DirectDequeueSize() < 0)
    {
        printf("Peek Error!\n");
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        printf("Peeking size is too Big\n");
        return -1;
    }

    if (iSize <= DirectDequeueSize())
    {
        memcpy_s(chpDest, iSize, &_buffer[_readPos], iSize);
    }
    else
    {
#ifdef C_DEBUG
        if (_writePos > _readPos ||
            ((_writePos < _readPos) &&
                ((_readPos + iSize) % _bufferSize > _writePos)))
            return -1;
#endif
        int size1 = DirectDequeueSize();
        int size2 = iSize - size1;
        memcpy_s(chpDest, size1, &_buffer[_readPos], size1);
        memcpy_s(&chpDest[size1], size2, _buffer, size2);
    }

    return iSize;
}


void RingBuffer::ClearBuffer(void)
{
    _readPos = 0;
    _writePos = 0;
    _useSize = 0;
    _freeSize = _bufferSize;
}


bool RingBuffer::Resize(int iSize)
{
    if (iSize > MAX_BUF_SIZE)
    {
        printf("Size is too Big!!!\n");
        return false;
    }

    if (iSize < _useSize)
    {
        printf("Size is Small than UseSize!!!\n");
        return false;
    }

    char* newBuffer = new char[iSize]();

    if (_writePos > _readPos)
    {
        memcpy_s(newBuffer, iSize, &_buffer[_readPos], _useSize);
    }
    else if (_writePos < _readPos)
    {
        int ReadSize = _writePos;
        int backSize = _useSize - ReadSize;
        memcpy_s(newBuffer, iSize, &_buffer[_readPos], backSize);
        memcpy_s(&newBuffer[backSize], iSize - backSize, _buffer, ReadSize);
    }

    delete[] _buffer;
    _buffer = newBuffer;

    _bufferSize = iSize;
    _freeSize = _bufferSize - _useSize;
    _readPos = 0;
    _writePos = _useSize;

#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return -1;
#endif
    return true;
}

int RingBuffer::MoveReadPos(int iSize)
{
#ifdef C_DEBUG
    if (GetUseSize() < 0)
    {
        printf("Move Read Pos Error!\n");
        return -1;
    }
#endif
    if (iSize > GetUseSize())
    {
        printf("Move Read Pos Size is too Big\n");
        return -1;
    }

#ifdef C_DEBUG
    if (iSize <= DirectDequeueSize() &&
        (_writePos > _readPos ||
            ((_writePos < _readPos) &&
                ((_readPos + iSize) % _bufferSize > _writePos))))
        return -1;
#endif
    _readPos = (_readPos + iSize) % _bufferSize;
    _useSize -= iSize;
    _freeSize += iSize;

#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return -1;
#endif
    return iSize;
}

int RingBuffer::MoveWritePos(int iSize)
{
#ifdef C_DEBUG
    if (GetFreeSize() < 0)
    {
        printf("Enqueue Error!\n");
        return -1;
    }
#endif
    if (iSize > GetFreeSize())
    {
        if (!Resize(_bufferSize + iSize))
            return -1;
    }
#ifdef C_DEBUG
    if (iSize > DirectEnqueueSize() &&
        (_writePos < _readPos ||
            ((_writePos > _readPos) &&
                ((_writePos + iSize) % _bufferSize >= _readPos))))
        return -1;
#endif
    _writePos = (_writePos + iSize) % _bufferSize;
    _useSize += iSize;
    _freeSize -= iSize;

#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return -1;
#endif
    return iSize;
}

char* RingBuffer::GetReadBufferPtr(void)
{
#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return nullptr;
#endif
    return &_buffer[_readPos];
}

char* RingBuffer::GetWriteBufferPtr(void)
{
#ifdef C_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
        return nullptr;
#endif
    return &_buffer[_writePos];
}
