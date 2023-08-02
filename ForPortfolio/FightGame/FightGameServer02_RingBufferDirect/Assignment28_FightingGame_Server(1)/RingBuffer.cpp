#include "RingBuffer.h"
#include <iostream>
//#define RINGBUFFER_DEBUG
//#define ENQ_DEQ_DEBUG

RingBuffer::RingBuffer(void) : _bufferSize(DEFAULT_BUF_SIZE), _freeSize(DEFAULT_BUF_SIZE - 1)
{
    _buffer = new char[_bufferSize]();
}

RingBuffer::RingBuffer(int bufferSize) : _bufferSize(bufferSize), _freeSize(bufferSize - 1)
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
#ifdef RINGBUFFER_DEBUG
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
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        printf("\n\n");
        printf("Buffer Size: %d\n", _bufferSize);
        printf("Read: %d\n", _readPos);
        printf("Write: %d\n", _writePos);
        printf("Real Use Size: %d\n", _useSize);
        printf("Real Free Size: %d\n", _freeSize);
        printf("Calced Use Size: %d\n", useSize);
        printf("\n");
        return -1;
    }

#endif
    return _useSize;
}

int RingBuffer::GetFreeSize(void)
{
#ifdef RINGBUFFER_DEBUG

    int freeSize = _bufferSize - 1;

    if (_writePos > _readPos)
    {
        freeSize = _bufferSize - _writePos + _readPos - 1;
    }
    else if (_writePos < _readPos)
    {
        freeSize = _readPos - _writePos - 1;
    }

    if (freeSize != _freeSize)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        printf("\n\n");
        printf("Buffer Size: %d\n", _bufferSize);
        printf("Read: %d\n", _readPos);
        printf("Write: %d\n", _writePos);
        printf("Real Use Size: %d\n", _useSize);
        printf("Real Free Size: %d\n", _freeSize);
        printf("Calced Free Size: %d\n", freeSize);
        printf("\n");
        return -1;
    }
#endif
    return _freeSize;
}

int RingBuffer::DirectEnqueueSize(void)
{
    int directEnqueueSize = -1;

    if (_writePos >= _readPos)
    {
        directEnqueueSize = _bufferSize - _writePos - 1;
    }
    else
    {
        directEnqueueSize = _readPos - _writePos - 1;
    }

    return directEnqueueSize;
}

int RingBuffer::DirectDequeueSize(void)
{
    int directDequeueSize = -1;

    if (_writePos >= _readPos)
    {
        directDequeueSize = _writePos - _readPos;
    }
    else
    {
        directDequeueSize = _bufferSize - _readPos - 1;
    }

    return directDequeueSize;
}


int RingBuffer::Enqueue(char* chpData, int iSize)
{
#ifdef RINGBUFFER_DEBUG
    if (GetFreeSize() < 0 || DirectEnqueueSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetFreeSize())
    {
        if (!Resize(_bufferSize + iSize))
        {
            printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
            return -1;
        }
    }

    if (iSize <= DirectEnqueueSize())
    {
        memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], iSize, chpData, iSize);

#ifdef ENQ_DEQ_DEBUG
        printf("\n\n");
        printf("Enqueue===========================\n\n");
        int idx = 0;
        while (idx < iSize)
        {
            printf("%d ", chpData[idx]);
            idx++;
        }
        printf("\n");

        idx = 0;
        while (idx < iSize)
        {
            printf("%d ", _buffer[(_writePos + 1) % _bufferSize + idx]);
            idx++;
        }
        printf("\n\n===================================");
        printf("\n\n");
#endif
    }
    else
    {
        int size1 = DirectEnqueueSize();
        int size2 = iSize - size1;
        memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], size1, chpData, size1);
        memcpy_s(_buffer, size2, &chpData[size1], size2);

#ifdef ENQ_DEQ_DEBUG
        printf("\n\n");
        printf("Enqueue===========================\n\n");
        int idx = 0;
        while (idx < iSize)
        {
            printf("%d ", chpData[idx]);
            idx++;
        }
        printf("\n");

        idx = 0;
        while (idx < size1)
        {
            printf("%d ", _buffer[(_writePos + 1) % _bufferSize + idx]);
            idx++;
        }
        printf("=[%d]", (_writePos + 1) % _bufferSize + idx - 1);
        idx = 0;
        while (idx < size2)
        {
            printf("%d ", _buffer[idx]);
            idx++;
        }
        printf("\n\n===================================");
        printf("\n\n");
#endif
    }

    _useSize += iSize;
    _freeSize -= iSize;
    _writePos = (_writePos + iSize) % _bufferSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif

    return iSize;
}

int RingBuffer::Dequeue(char* chpData, int iSize)
{
#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || DirectDequeueSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }

    if (iSize <= DirectDequeueSize())
    {
        memcpy_s(chpData, iSize, &_buffer[(_readPos + 1) % _bufferSize], iSize);

#ifdef ENQ_DEQ_DEBUG
        printf("\n\n");
        printf("Dequeue===========================\n\n");
        int idx = 0;
        while (idx < iSize)
        {
            printf("%d ", _buffer[(_readPos + 1) % _bufferSize + idx]);
            idx++;
        }
        printf("\n\n===================================");
        printf("\n\n");
#endif
    }
    else
    {
        int size1 = DirectDequeueSize();
        int size2 = iSize - size1;
        memcpy_s(chpData, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
        memcpy_s(&chpData[size1], size2, _buffer, size2);

#ifdef ENQ_DEQ_DEBUG
        printf("\n\n");
        printf("Dequeue===========================\n\n");
        int idx = 0;
        while (idx < size1)
        {
            printf("%d ", _buffer[(_readPos + 1) % _bufferSize + idx]);
            idx++;
        }
        printf("=[%d]", (_readPos + 1) % _bufferSize + idx - 1);

        idx = 0;
        while (idx < size2)
        {
            printf("%d ", _buffer[idx]);
            idx++;
        }
        printf("\n\n===================================");
        printf("\n\n");
#endif
    }

    _useSize -= iSize;
    _freeSize += iSize;
    _readPos = (_readPos + iSize) % _bufferSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif
    return iSize;
}

int RingBuffer::Peek(char* chpDest, int iSize)
{
#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || DirectDequeueSize() < 0)
    {
        printf("Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        printf("Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }

    if (iSize <= DirectDequeueSize())
    {
        memcpy_s(chpDest, iSize, &_buffer[(_readPos + 1) % _bufferSize], iSize);
    }
    else
    {
        int size1 = DirectDequeueSize();
        int size2 = iSize - size1;
        memcpy_s(chpDest, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
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
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return false;
    }

    if (iSize < _useSize)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return false;
    }

    char* newBuffer = new char[iSize]();

    if (_writePos > _readPos)
    {
        memcpy_s(newBuffer, iSize, &_buffer[(_readPos + 1) % _bufferSize], _useSize);
    }
    else if (_writePos < _readPos)
    {
        int size1 = _bufferSize - _readPos - 1;
        int size2 = _writePos + 1;
        memcpy_s(newBuffer, iSize, &_buffer[(_readPos + 1) % _bufferSize], size1);
        memcpy_s(&newBuffer[size1], iSize - size1, _buffer, size2);
    }

    delete[] _buffer;
    _buffer = newBuffer;
    _bufferSize = iSize;
    _freeSize = _bufferSize - _useSize - 1;
    _readPos = 0;
    _writePos = _useSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return false;
    }
#endif
    return true;
}

int RingBuffer::MoveReadPos(int iSize)
{
#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }

    _readPos = (_readPos + iSize) % _bufferSize;
    _useSize -= iSize;
    _freeSize += iSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif
    return iSize;
}

int RingBuffer::MoveWritePos(int iSize)
{
#ifdef RINGBUFFER_DEBUG
    if (GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetFreeSize())
    {
        if (!Resize(_bufferSize + iSize))
        {
            printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
            return -1;
        }
    }

    _writePos = (_writePos + iSize) % _bufferSize;
    _useSize += iSize;
    _freeSize -= iSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return -1;
    }
#endif
    return iSize;
}

char* RingBuffer::GetReadBufferPtr(void)
{
#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return nullptr;
    }
#endif
    return &_buffer[_readPos + 1];
}

char* RingBuffer::GetWriteBufferPtr(void)
{
#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        printf("Error! Func %s Function %s Line %d\n", __func__, __func__, __LINE__);
        return nullptr;
    }
#endif
    return &_buffer[_writePos + 1];
}

void RingBuffer::GetBufferDataForDebug()
{
    printf("\n");
    printf("Buffer Size: %d\n", _bufferSize);
    printf("Read: %d\n", _readPos);
    printf("Write: %d\n", _writePos);
    printf("Real Use Size: %d\n", _useSize);
    printf("Real Free Size: %d\n", _freeSize);
    printf("Direct Dequeue Size: %d\n", DirectDequeueSize());
    printf("Direct Enqueue Size: %d\n", DirectEnqueueSize());
    printf("\n");
}