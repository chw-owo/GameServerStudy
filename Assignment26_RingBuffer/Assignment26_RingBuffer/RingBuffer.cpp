#include "RingBuffer.h"

RingBuffer::RingBuffer(void)
{
}

RingBuffer::RingBuffer(int iBufferSize)
{
}

void RingBuffer::Resize(int size)
{
}

int RingBuffer::GetBufferSize(void)
{
    return 0;
}

int RingBuffer::GetUseSize(void)
{
    return 0;
}

int RingBuffer::GetFreeSize(void)
{
    return 0;
}

int RingBuffer::Enqueue(char* chpData, int iSize)
{
    return 0;
}

int RingBuffer::Dequeue(char* chpData, int iSize)
{
    return 0;
}

int RingBuffer::Peek(char* chpDest, int iSize)
{
    return 0;
}

int RingBuffer::DirectEnqueueSize(void)
{
    return 0;
}

int RingBuffer::DirectDequeueSize(void)
{
    return 0;
}

void RingBuffer::ClearBuffer(void)
{
}

int RingBuffer::MoveRear(int iSize)
{
    return 0;
}

int RingBuffer::MoveFront(int iSize)
{
    return 0;
}

char* RingBuffer::GetFrontBufferPtr(void)
{
    return nullptr;
}

char* RingBuffer::GetRearBufferPtr(void)
{
    return nullptr;
}
