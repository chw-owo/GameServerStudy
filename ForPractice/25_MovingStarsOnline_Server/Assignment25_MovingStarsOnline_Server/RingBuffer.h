#pragma once
#define DEFAULT_BUF_SIZE 512
#define MAX_BUF_SIZE 4096

class RingBuffer
{
public:
    RingBuffer(void);
    RingBuffer(int iBufferSize);
    ~RingBuffer(void);

    int GetBufferSize(void);
    int GetUseSize(void);
    int GetFreeSize(void);
    int DirectEnqueueSize(void);
    int DirectDequeueSize(void);

    int Enqueue(char* chpData, int iSize);
    int Dequeue(char* chpData, int iSize);
    int Peek(char* chpDest, int iSize);
    void ClearBuffer(void);
    bool Resize(int size);
     
    int MoveReadPos(int iSize);
    int MoveWritePos(int iSize);
    char* GetReadBufferPtr(void);
    char* GetWriteBufferPtr(void);

private:
    char* _buffer;
    int _bufferSize;

    int _useSize = 0;
    int _freeSize = 0;

    int _readPos = 0;
    int _writePos = 0;

};

