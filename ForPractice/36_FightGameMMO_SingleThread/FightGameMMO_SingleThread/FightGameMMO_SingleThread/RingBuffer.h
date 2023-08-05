#pragma once
#include <iostream>

#define DEFAULT_BUF_SIZE (16384 + 1) 
#define MAX_BUF_SIZE (32768 + 1) 

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
    RingBuffer(void);
    RingBuffer(int iBufferSize);
    ~RingBuffer(void);

    bool Resize(int iSize);
    void ClearBuffer(void);
    int Peek(char* chpDest, int iSize);
    int Enqueue(char* chpData, int iSize);
    int Dequeue(char* chpData, int iSize);
    int MoveReadPos(int iSize);
    int MoveWritePos(int iSize);


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

private:
    char* _buffer;
    int _readPos = 0;
    int _writePos = 0;

    int _bufferSize;
    int _useSize = 0;
    int _freeSize = 0;
};

