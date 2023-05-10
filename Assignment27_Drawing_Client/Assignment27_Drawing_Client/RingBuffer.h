#pragma once
#define DEFAULT_BUF_SIZE (4096 + 1)
#define MAX_BUF_SIZE (32768 + 1) 

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
    bool Resize(int iSize);

    int MoveReadPos(int iSize);
    int MoveWritePos(int iSize);
    char* GetReadBufferPtr(void);
    char* GetWriteBufferPtr(void);

    // For Debug
    void GetBufferDataForDebug();

private:
    char* _buffer;
    int _bufferSize;

    int _useSize = 0;
    int _freeSize = 0;

    int _readPos = 0;
    int _writePos = 0;

    // readPos는 비어있는 공간을,
    // writePos는 마지막으로 넣은 공간을 가리킨다
    // 따라서 readPos == writePos는 버퍼가 비어있음을 의미하고
    // 버퍼가 찼을 때는 (readPos + 1)%_bufferSize == writePos 가 된다. 
};

