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
class CRingBuffer
{
public:
    inline CRingBuffer(void) 
        : _initBufferSize(DEFAULT_BUF_SIZE), _bufferSize(DEFAULT_BUF_SIZE), _freeSize(DEFAULT_BUF_SIZE - 1)
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


    inline void ClearBuffer(void)
    {
        _readPos = 0;
        _writePos = 0;
        _useSize = 0;
        _bufferSize = _initBufferSize;
        _freeSize = _bufferSize;
    }

    inline int Peek(char* chpDest, int size)
    {
        if (size > _useSize)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d < req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            ::wprintf(L"%s[%d]: used size %d < req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            return -1;
        }

        int directDequeueSize = DirectDequeueSize();
        if (size <= directDequeueSize)
        {
            memcpy_s(chpDest, size, &_buffer[(_readPos + 1) % _bufferSize], size);
        }
        else
        {
            int size1 = directDequeueSize;
            int size2 = size - size1;
            memcpy_s(chpDest, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
            memcpy_s(&chpDest[size1], size2, _buffer, size2);
        }

        return size;
    }

    inline int Enqueue(char* chpData, int size)
    {
        if (size > MAX_BUF_SIZE)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d] req %d, max %d",
                _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);

            ::wprintf(L"%s[%d] req %d, max %d",
                _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);

            return -1;
        }
        else if (size > _freeSize)
        {
            if (!Resize(_bufferSize + (int)(size * 1.5f)))
            {          
                LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                    L"%s[%d] Fail to Resize",
                    _T(__FUNCTION__), __LINE__);

                ::wprintf(L"%s[%d] Fail to Resize",
                    _T(__FUNCTION__), __LINE__);

                return -1;
            }
        }

        int directEnqueueSize = DirectEnqueueSize();
        if (size <= directEnqueueSize)
        {
            memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], size, chpData, size);
        }
        else
        {
            int size1 = directEnqueueSize;
            int size2 = size - size1;
            memcpy_s(&_buffer[(_writePos + 1) % _bufferSize], size1, chpData, size1);
            memcpy_s(_buffer, size2, &chpData[size1], size2);
        }

        _useSize += size;
        _freeSize -= size;
        _writePos = (_writePos + size) % _bufferSize;

        return size;
    }

    inline int Dequeue(char* chpData, int size)
    {
        if (size > _useSize)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            ::wprintf(L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            return -1;
        }

        int directDequeueSize = DirectDequeueSize();
        if (size <= directDequeueSize)
        {
            memcpy_s(chpData, size, &_buffer[(_readPos + 1) % _bufferSize], size);
        }
        else
        {
            int size1 = directDequeueSize;
            int size2 = size - size1;
            memcpy_s(chpData, size1, &_buffer[(_readPos + 1) % _bufferSize], size1);
            memcpy_s(&chpData[size1], size2, _buffer, size2);
        }

        _useSize -= size;
        _freeSize += size;
        _readPos = (_readPos + size) % _bufferSize;

        return size;
    }

    inline int MoveReadPos(int size)
    {
        if (size > _useSize)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            ::wprintf(L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            return -1;
        }

        _readPos = (_readPos + size) % _bufferSize;
        _useSize -= size;
        _freeSize += size;

        return size;
    }

    inline int MoveWritePos(int size)
    {
        if (size > MAX_BUF_SIZE)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d] req %d, max %d",
                _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);
            ::wprintf(L"%s[%d] req %d, max %d",
                _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);

            return -1;
        }
        else if (size > _freeSize)
        {
            if (!Resize(_bufferSize + (int)(size * 1.5f)))
            {
                LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                    L"%s[%d] Fail to Resize",
                    _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);
                ::wprintf(L"%s[%d] Fail to Resize",
                    _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);

                return -1;
            }
        }

        _writePos = (_writePos + size) % _bufferSize;
        _useSize += size;
        _freeSize -= size;

        return size;
    }

    inline int GetBufferSize(void) { return _bufferSize; }
    inline int GetUseSize(void) { return _useSize; }
    inline int GetFreeSize(void) { return _freeSize; }
    inline char* GetReadPtr(void) { return &_buffer[_readPos + 1]; }
    inline char* GetWritePtr(void) { return &_buffer[_writePos + 1]; }
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

    inline bool Resize(int size)
    {
        if (size > MAX_BUF_SIZE)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d]: req %d, max %d\n",
                _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);

            ::wprintf(L"%s[%d]: req %d, max %d\n",
                _T(__FUNCTION__), __LINE__, size, MAX_BUF_SIZE);

            return false;
        }

        if (size < _useSize)
        {
            LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
                L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            ::wprintf(L"%s[%d]: used size %d, req size %d\n",
                _T(__FUNCTION__), __LINE__, _useSize, size);

            return false;
        }

        char* newBuffer = new char[size];

        if (_writePos > _readPos)
        {
            memcpy_s(newBuffer, size, &_buffer[_readPos % _bufferSize], _useSize + 1);
        }
        else if (_writePos < _readPos)
        {
            int size1 = _bufferSize - _readPos;
            int size2 = _writePos + 1;
            memcpy_s(newBuffer, size, &_buffer[_readPos % _bufferSize], size1);
            memcpy_s(&newBuffer[size1], size - size1, _buffer, size2);
        }

        delete[] _buffer;
        _buffer = newBuffer;
        _bufferSize = size;
        _freeSize = _bufferSize - _useSize - 1;
        _readPos = 0;
        _writePos = _useSize;

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
};

