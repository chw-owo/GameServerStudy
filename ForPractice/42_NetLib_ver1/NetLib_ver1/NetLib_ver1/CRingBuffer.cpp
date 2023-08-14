#include "CRingBuffer.h"
#include <iostream>

CRingBuffer::CRingBuffer(void) : _bufferSize(DEFAULT_BUF_SIZE), _freeSize(DEFAULT_BUF_SIZE - 1)
{
    _buffer = new char[_bufferSize];
}

CRingBuffer::CRingBuffer(int bufferSize) : _bufferSize(bufferSize), _freeSize(bufferSize - 1)
{
    _buffer = new char[_bufferSize];
}

CRingBuffer::~CRingBuffer(void)
{
    delete[] _buffer;
}

int CRingBuffer::GetBufferSize(void)
{
    return _bufferSize;
}

int CRingBuffer::GetUseSize(void)
{
    return _useSize;
}

int CRingBuffer::GetFreeSize(void)
{
    return _freeSize;
}

int CRingBuffer::DirectEnqueueSize(void)
{
    int directEnqueueSize;

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

int CRingBuffer::DirectDequeueSize(void)
{
    int directDequeueSize;

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


int CRingBuffer::Enqueue(char* chpData, int iSize)
{
    if (iSize > _freeSize)
    {
        if (!Resize(_bufferSize + (int)(iSize * 1.5f)))
        {
            ::printf("Error! Func %s Line %d\n", __func__, __LINE__);
            return -1;
        }
    }

    int directEnqueueSize = DirectEnqueueSize();
    if (iSize <= directEnqueueSize)
    {
        memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], iSize, chpData, iSize);

        /*
        ::printf("CRingBuffer Input: ");
        for (int i = 0; i < iSize; i++)
            ::printf("%x ", chpData[i]);

        ::printf("\nCRingBuffer: ");
        for (int i = 0; i < iSize; i++)
            ::printf("%x ", _buffer[((_writePos + 1) % _bufferSize) + i]);

        ::printf("\n");
        */
    }
    else
    {
        int size1 = directEnqueueSize;
        int size2 = iSize - size1;
        memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], size1, chpData, size1);
        memcpy_s(_buffer, size2, &chpData[size1], size2);

        /*
        ::printf("CRingBuffer Input: ");
        for (int i = 0; i < iSize; i++)
            ::printf("%x ", chpData[i]);

        ::printf("\nCRingBuffer: ");
        for (int i = 0; i < size1; i++)
            ::printf("%x ", _buffer[((_writePos + 1) % _bufferSize) + i]);
        for (int i = 0; i < size2; i++)
            ::printf("%x ", _buffer[i]);

        ::printf("\n");
        */
    }

    _useSize += iSize;
    _freeSize -= iSize;
    _writePos = (_writePos + iSize) % _bufferSize;

    return iSize;
}

int CRingBuffer::Dequeue(char* chpData, int iSize)
{
    if (iSize > _useSize || iSize < 0)
    {
        ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
            __func__, __LINE__, _useSize, iSize);
        return -1;
    }

    int directDequeueSize = DirectDequeueSize();
    if (iSize <= directDequeueSize)
    {
        memcpy_s(chpData, iSize, &_buffer[(_readPos + 1) % _bufferSize], iSize);
    }
    else
    {
        int size1 = directDequeueSize;
        int size2 = iSize - size1;
        memcpy_s(chpData, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
        memcpy_s(&chpData[size1], size2, _buffer, size2);
    }

    _useSize -= iSize;
    _freeSize += iSize;
    _readPos = (_readPos + iSize) % _bufferSize;

    return iSize;
}

int CRingBuffer::Peek(char* chpDest, int iSize)
{
    if (iSize > _useSize)
    {
        ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
            __func__, __LINE__, _useSize, iSize);
        return -1;
    }

    int directDequeueSize = DirectDequeueSize();
    if (iSize <= directDequeueSize)
    {
        memcpy_s(chpDest, iSize, &_buffer[(_readPos + 1) % _bufferSize], iSize);
    }
    else
    {
        int size1 = directDequeueSize;
        int size2 = iSize - size1;
        memcpy_s(chpDest, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
        memcpy_s(&chpDest[size1], size2, _buffer, size2);
    }

    return iSize;
}


void CRingBuffer::ClearBuffer(void)
{
    _readPos = 0;
    _writePos = 0;
    _useSize = 0;
    _freeSize = _bufferSize;
}


bool CRingBuffer::Resize(int iSize)
{
    if (iSize > MAX_BUF_SIZE)
    {
        ::printf("Error! Func %s Line %d (max size - %d, req size - %d)\n",
            __func__, __LINE__, MAX_BUF_SIZE, iSize);
        return false;
    }

    if (iSize < _useSize)
    {
        ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
            __func__, __LINE__, _useSize, iSize);
        return false;
    }

    char* newBuffer = new char[iSize];

    if (_writePos > _readPos)
    {
        memcpy_s(newBuffer, iSize, &_buffer[_readPos % _bufferSize], _useSize + 1);
    }
    else if (_writePos < _readPos)
    {
        int size1 = _bufferSize - _readPos;
        int size2 = _writePos + 1;
        memcpy_s(newBuffer, iSize, &_buffer[_readPos % _bufferSize], size1);
        memcpy_s(&newBuffer[size1], iSize - size1, _buffer, size2);
    }

    delete[] _buffer;
    _buffer = newBuffer;
    _bufferSize = iSize;
    _freeSize = _bufferSize - _useSize - 1;
    _readPos = 0;
    _writePos = _useSize;

    return true;
}

int CRingBuffer::MoveReadPos(int iSize)
{
    if (iSize > _useSize)
    {
        ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
            __func__, __LINE__, _useSize, iSize);
        return -1;
    }

    _readPos = (_readPos + iSize) % _bufferSize;
    _useSize -= iSize;
    _freeSize += iSize;

    return iSize;
}

int CRingBuffer::MoveWritePos(int iSize)
{
    if (iSize > _freeSize)
    {
        if (!Resize(_bufferSize + (int)(iSize * 1.5f)))
        {
            ::printf("Error! Func %s Line %d\n", __func__, __LINE__);
            return -1;
        }
    }

    _writePos = (_writePos + iSize) % _bufferSize;
    _useSize += iSize;
    _freeSize -= iSize;

    return iSize;
}

char* CRingBuffer::GetBufferFrontPtr(void)
{
    return &_buffer[0];
}

char* CRingBuffer::GetReadBufferPtr(void)
{
    return &_buffer[_readPos + 1];
}

char* CRingBuffer::GetWriteBufferPtr(void)
{
    return &_buffer[_writePos + 1];
}

void CRingBuffer::GetBufferDataForDebug()
{
    ::printf("\n");
    ::printf("Buffer Size: %d\n", _bufferSize);
    ::printf("Read: %d\n", _readPos);
    ::printf("Write: %d\n", _writePos);
    ::printf("Real Use Size: %d\n", _useSize);
    ::printf("Real Free Size: %d\n", _freeSize);
    ::printf("Direct Dequeue Size: %d\n", DirectDequeueSize());
    ::printf("Direct Enqueue Size: %d\n", DirectEnqueueSize());
    ::printf("\n");
}