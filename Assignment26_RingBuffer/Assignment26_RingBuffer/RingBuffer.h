#pragma once
#define SIZE 100
class RingBuffer
{
public:
    RingBuffer(void);
    RingBuffer(int iBufferSize);

    void Resize(int size);
    int GetBufferSize(void);
    int GetUseSize(void);
    int GetFreeSize(void);

    int Enqueue(char* chpData, int iSize);
    int Dequeue(char* chpData, int iSize);
    int Peek(char* chpDest, int iSize);
    int DirectEnqueueSize(void);
    int DirectDequeueSize(void);

    void ClearBuffer(void);
    int MoveRear(int iSize);
    int MoveFront(int iSize);
    char* GetFrontBufferPtr(void);
    char* GetRearBufferPtr(void);

private:
    char Buffer[SIZE] = { 0, };
    char* front;
    char* rear;
};

