#pragma once
#include "CTlsPool.h"

template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
#define __KEY_BIT__ 9
#define __ADDRESS_BIT__ 47
    unsigned __int64 _addressMask = 0;
    unsigned int _keyMask = 0;
    volatile __int64 _key = 0;

public:
    CLockFreeQueue()
    {
        _pPool = new CTlsPool<QueueNode>(0, true);
        _keyMask = 0b11111111111111111;
        _addressMask = 0b11111111111111111;
        _addressMask <<= __ADDRESS_BIT__;
        _addressMask = ~_addressMask;

        QueueNode* node = _pPool->Alloc();
        node->_next = NULL;

        //For Protect ABA 
        unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
        key <<= __ADDRESS_BIT__;
        _head = (__int64)node;
        _head &= _addressMask;
        _head |= key;
        _tail = _head;
        _size = 0;
    }

private:
    class QueueNode
    {
        friend CLockFreeQueue;

        T _data = 0;
        __int64 _next = 0;
    };

    __int64 _head;
    __int64 _tail;

private:
    volatile long _size;
    CTlsPool<QueueNode>* _pPool;

public:
    void Enqueue(T data)
    {
        QueueNode* node = _pPool->Alloc();
        node->_data = data;
        node->_next = NULL;

        unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
        key <<= __ADDRESS_BIT__;
        __int64 newNode = (__int64)node;
        newNode &= _addressMask;
        newNode |= key;

        while (true)
        {
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;

            if (next != NULL)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }
            else
            {
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {
                    InterlockedCompareExchange64(&_tail, newNode, tail);
                    break;
                }
            }
        }

        InterlockedIncrement(&_size);
    }

    T Dequeue()
    {
        T data = 0;
        
        while (true)
        {
            __int64 tail = _tail;
            __int64 head = _head;
            QueueNode* headNode = (QueueNode*)(head & _addressMask);
            __int64 next = headNode->_next;

            if (_size == 0) return data;
            if (next == NULL) return data;

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                data = headNode->_data;
                _pPool->Free(headNode);
                break;
            }

        }

        InterlockedDecrement(&_size);
        return data;
    }
};

