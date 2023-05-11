#include "RingBuffer.h"
#include <iostream>
//#define RINGBUFFER_DEBUG

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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        wprintf(L"\n\n");
        wprintf(L"Buffer Size: %d\n", _bufferSize);
        wprintf(L"Read: %d\n", _readPos);
        wprintf(L"Write: %d\n", _writePos);
        wprintf(L"Real Use Size: %d\n", _useSize);
        wprintf(L"Real Free Size: %d\n", _freeSize);
        wprintf(L"Calced Use Size: %d\n", useSize);
        wprintf(L"\n");
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        wprintf(L"\n\n");
        wprintf(L"Buffer Size: %d\n", _bufferSize);
        wprintf(L"Read: %d\n", _readPos);
        wprintf(L"Write: %d\n", _writePos);
        wprintf(L"Real Use Size: %d\n", _useSize);
        wprintf(L"Real Free Size: %d\n", _freeSize);
        wprintf(L"Calced Free Size: %d\n", freeSize);
        wprintf(L"\n");
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetFreeSize())
    {
        if (!Resize(_bufferSize + iSize))
        {
            wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
            return -1;
        }
    }

    if (iSize <= DirectEnqueueSize())
    {
        memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], iSize, chpData, iSize);

#ifdef RINGBUFFER_DEBUG
        wprintf(L"\n\n");
        wprintf(L"Enqueue===========================\n\n");
        int idx = 0;
        while (idx < iSize)
        {
            wprintf(L"%d ", chpData[idx]);
            idx++;
        }
        wprintf(L"\n");

        idx = 0;
        while (idx < iSize)
        {
            wprintf(L"%d ", _buffer[(_writePos + 1) % _bufferSize + idx]);
            idx++;
        }
        wprintf(L"\n\n===================================");
        wprintf(L"\n\n");
#endif
    }
    else
    {
        int size1 = DirectEnqueueSize();
        int size2 = iSize - size1;
        memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], size1, chpData, size1);
        memcpy_s(_buffer, size2, &chpData[size1], size2);

#ifdef RINGBUFFER_DEBUG
        wprintf(L"\n\n");
        wprintf(L"Enqueue===========================\n\n");
        int idx = 0;
        while (idx < iSize)
        {
            wprintf(L"%d ", chpData[idx]);
            idx++;
        }
        wprintf(L"\n");

        idx = 0;
        while (idx < size1)
        {
            wprintf(L"%d ", _buffer[(_writePos + 1) % _bufferSize + idx]);
            idx++;
        }
        wprintf(L"=[%d]", (_writePos + 1) % _bufferSize + idx - 1);
        idx = 0;
        while (idx < size2)
        {
            wprintf(L"%d ", _buffer[idx]);
            idx++;
        }
        wprintf(L"\n\n===================================");
        wprintf(L"\n\n");
#endif
    }

    _useSize += iSize;
    _freeSize -= iSize;
    _writePos = (_writePos + iSize) % _bufferSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }

    if (iSize <= DirectDequeueSize())
    {
        memcpy_s(chpData, iSize, &_buffer[(_readPos + 1) % _bufferSize], iSize);

#ifdef RINGBUFFER_DEBUG
        wprintf(L"\n\n");
        wprintf(L"Dequeue===========================\n\n");
        int idx = 0;
        while (idx < iSize)
        {
            wprintf(L"%d ", _buffer[(_readPos + 1) % _bufferSize + idx]);
            idx++;
        }
        wprintf(L"\n\n===================================");
        wprintf(L"\n\n");
#endif
    }
    else
    {
        int size1 = DirectDequeueSize();
        int size2 = iSize - size1;
        memcpy_s(chpData, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
        memcpy_s(&chpData[size1], size2, _buffer, size2);

#ifdef RINGBUFFER_DEBUG
        wprintf(L"\n\n");
        wprintf(L"Dequeue===========================\n\n");
        int idx = 0;
        while (idx < size1)
        {
            wprintf(L"%d ", _buffer[(_readPos + 1) % _bufferSize + idx]);
            idx++;
        }
        wprintf(L"=[%d]", (_readPos + 1) % _bufferSize + idx - 1);

        idx = 0;
        while (idx < size2)
        {
            wprintf(L"%d ", _buffer[idx]);
            idx++;
        }
        wprintf(L"\n\n===================================");
        wprintf(L"\n\n");
#endif
    }

    _useSize -= iSize;
    _freeSize += iSize;
    _readPos = (_readPos + iSize) % _bufferSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return false;
    }

    if (iSize < _useSize)
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetUseSize())
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }

    _readPos = (_readPos + iSize) % _bufferSize;
    _useSize -= iSize;
    _freeSize += iSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return -1;
    }
#endif

    if (iSize > GetFreeSize())
    {
        if (!Resize(_bufferSize + iSize))
        {
            wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
            return -1;
        }
    }

    _writePos = (_writePos + iSize) % _bufferSize;
    _useSize += iSize;
    _freeSize -= iSize;

#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
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
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return nullptr;
    }
#endif
    return &_buffer[_readPos];
}

char* RingBuffer::GetWriteBufferPtr(void)
{
#ifdef RINGBUFFER_DEBUG
    if (GetUseSize() < 0 || GetFreeSize() < 0)
    {
        wprintf(L"Error! Func %s Line %d\n", __func__, __LINE__);
        return nullptr;
    }
#endif
    return &_buffer[_writePos];
}

void RingBuffer::GetBufferDataForDebug()
{
    wprintf(L"\n");
    wprintf(L"Buffer Size: %d\n", _bufferSize);
    wprintf(L"Read: %d\n", _readPos);
    wprintf(L"Write: %d\n", _writePos);
    wprintf(L"Real Use Size: %d\n", _useSize);
    wprintf(L"Real Free Size: %d\n", _freeSize);
    wprintf(L"Direct Dequeue Size: %d\n", DirectDequeueSize());
    wprintf(L"Direct Enqueue Size: %d\n", DirectEnqueueSize());
    wprintf(L"\n");
}