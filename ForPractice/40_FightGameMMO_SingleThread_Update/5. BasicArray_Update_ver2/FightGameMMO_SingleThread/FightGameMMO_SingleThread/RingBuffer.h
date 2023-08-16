#pragma once
#include <iostream>
#include "SystemLog.h"
#include <tchar.h>

#define DEFAULT_BUF_SIZE (32768 + 1) 
#define MAX_BUF_SIZE (65536 + 1)

#define __RINGBUFFER_DEBUG__

/*====================================================================

    <Ring Buffer>

    readPos는 비어있는 공간을,
    writePos는 마지막으로 넣은 공간을 가리킨다
    따라서 readPos == writePos는 버퍼가 비어있음을 의미하고
    버퍼가 찼을 때는 (readPos + 1)%_bufferSize == writePos 가 된다.

======================================================================*/
class RingBuffer
{
public:
    inline RingBuffer(void) 
        : _bufferSize(DEFAULT_BUF_SIZE), _freeSize(DEFAULT_BUF_SIZE - 1)
    { 
        _buffer = new char[_bufferSize]; 
    }

    inline RingBuffer(int bufferSize) 
        : _bufferSize(bufferSize), _freeSize(bufferSize - 1)
    { 
        _buffer = new char[_bufferSize]; 
    }

    inline ~RingBuffer(void)
    {
        delete[] _buffer;
    }


    inline void ClearBuffer(void)
    {
        _readPos = 0;
        _writePos = 0;
        _useSize = 0;
        _freeSize = _bufferSize;
    }

    inline int Peek(char* chpDest, int iSize)
    {
        if (iSize > _useSize)
        {
            ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
                __func__, __LINE__, _useSize, iSize);
            LOG(L"ERROR", SystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, iSize);
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

    inline int Enqueue(char* chpData, int iSize)
    {
        if (iSize > _freeSize)
        {
            if (!Resize(_bufferSize + (int)(iSize * 1.5f)))
            {
                ::printf("Error! Func %s Line %d\n", __func__, __LINE__);
                LOG(L"ERROR", SystemLog::ERROR_LEVEL, L"%s[%d]", _T(__FUNCTION__), __LINE__);
                return -1;
            }
        }

        int directEnqueueSize = DirectEnqueueSize();
        if (iSize <= directEnqueueSize)
        {
            memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], iSize, chpData, iSize);
        }
        else
        {
            int size1 = directEnqueueSize;
            int size2 = iSize - size1;
            memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], size1, chpData, size1);
            memcpy_s(_buffer, size2, &chpData[size1], size2);
        }

        _useSize += iSize;
        _freeSize -= iSize;
        _writePos = (_writePos + iSize) % _bufferSize;

        return iSize;
    }

    inline int Dequeue(char* chpData, int iSize)
    {
        if (iSize > _useSize)
        {
            ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
                __func__, __LINE__, _useSize, iSize);
            LOG(L"ERROR", SystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, iSize);
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

    inline int MoveReadPos(int iSize)
    {
        if (iSize > _useSize)
        {
            ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
                __func__, __LINE__, _useSize, iSize);

            LOG(L"ERROR", SystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, iSize);

            return -1;
        }

        _readPos = (_readPos + iSize) % _bufferSize;
        _useSize -= iSize;
        _freeSize += iSize;

        return iSize;
    }

    inline int MoveWritePos(int iSize)
    {
        if (iSize > _freeSize)
        {
            if (!Resize(_bufferSize + (int)(iSize * 1.5f)))
            {
                ::printf("Error! Func %s Line %d\n", __func__, __LINE__);

                LOG(L"ERROR", SystemLog::ERROR_LEVEL,
                    L"%s[%d]\n", _T(__FUNCTION__), __LINE__);

                return -1;
            }
        }

        _writePos = (_writePos + iSize) % _bufferSize;
        _useSize += iSize;
        _freeSize -= iSize;

        return iSize;
    }

    inline int GetBufferSize(void) { return _bufferSize; }
    inline int GetUseSize(void) { return _useSize; }
    inline int GetFreeSize(void) { return _freeSize; }
    inline char* GetReadBufferPtr(void) { return &_buffer[_readPos + 1]; }
    inline char* GetWriteBufferPtr(void) { return &_buffer[_writePos + 1]; }
    inline int DirectEnqueueSize(void)
    {
        if (_writePos >= _readPos)
            return _bufferSize - _writePos - 1;
        else
            return _readPos - _writePos - 1;
    }

    inline int DirectDequeueSize(void)
    {
        if (_writePos >= _readPos)
            return _writePos - _readPos;
        else
            return _bufferSize - _readPos - 1;
    }

    // For Debug
    inline void GetBufferDataForDebug()
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

    inline bool Resize(int iSize)
    {
        if (iSize > MAX_BUF_SIZE)
        {
            ::printf("Error! Func %s Line %d (max size - %d, req size - %d)\n",
                __func__, __LINE__, MAX_BUF_SIZE, iSize);
            LOG(L"ERROR", SystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, iSize);
            return false;
        }

        if (iSize < _useSize)
        {
            ::printf("Error! Func %s Line %d (used size - %d, req size - %d)\n",
                __func__, __LINE__, _useSize, iSize);

            LOG(L"ERROR", SystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, iSize);

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

private:
    char* _buffer;
    int _readPos = 0;
    int _writePos = 0;

    int _bufferSize;
    int _useSize = 0;
    int _freeSize = 0;
};

