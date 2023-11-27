#pragma once
#include "CTlsPool.h"
#include <iostream>
#include <cstdarg>
using namespace std;

template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
    // QID 13 bit (max 8192), Key 4 bit (max 16), Address 47 bit
#define __KEY_BIT__ 4
#define __ADDRESS_BIT__ 47
    static unsigned __int64 _QIDMask;
    unsigned __int64 _keyAddressMask = 0;
    unsigned __int64 _addressMask = 0;
    short _key = 0;

public:
    class QueueNode
    {
        friend CLockFreeQueue;

    public:
        QueueNode() { __debugbreak(); }
        QueueNode(short QID)
        {
            _next = (__int64)QID;
            _next <<= (__KEY_BIT__ + __ADDRESS_BIT__);
            _next &= CLockFreeQueue::_QIDMask;
        }

        ~QueueNode()
        {
            _data = 0;
            _next = 0;
        }

    private:
        T _data = 0;
        __int64 _next = 0;
    };

public:
    static short _QIDSupplier;
    static CTlsPool<QueueNode>* _pQueuePool;

private:
    __int64 _head;
    __int64 _tail;
    long _useSize;
    short _QID;

private:
    inline __int64 CreateAddress(QueueNode* node)
    {
        unsigned __int64 QID = (unsigned __int64)_QID;
        QID <<= (__KEY_BIT__ + __ADDRESS_BIT__);
        QID &= _QIDMask;

        unsigned __int64 key = (unsigned __int64)InterlockedIncrement16(&_key);
        key <<= __ADDRESS_BIT__;
        key &= _keyAddressMask;
        key |= QID;

        __int64 address = (__int64)node;
        address &= _addressMask;
        address |= key;
        return address;
    }

    inline short GetQID(__int64 next)
    {
        short QID = (((unsigned __int64)next) >> (__KEY_BIT__ + __ADDRESS_BIT__));
        QID &= 0b1111111111111;
        return QID;
    }

public:
    long GetUseSize() { return _useSize; }

public:
    CLockFreeQueue()
    {
        _QID = InterlockedIncrement16(&_QIDSupplier);

        _keyAddressMask = ~_QIDMask;
        _addressMask = 0b11111111111111111;
        _addressMask <<= __ADDRESS_BIT__;
        _addressMask = ~_addressMask;

        _QID &= 0b1111111111111;
        QueueNode* node = _pQueuePool->Alloc(_QID);
        _head = CreateAddress(node);
        _tail = _head;
        _useSize = 0;
    }

public:
    void Enqueue(T data)
    {
        QueueNode* node = _pQueuePool->Alloc(_QID);
        node->_data = data;
        __int64 newNode = CreateAddress(node);

        for (;;)
        {
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;

            short QID = GetQID(next);
            if (QID != _QID) continue; // Check where tailNode is allocated

            if ((next & _addressMask) != NULL)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }
            else
            {
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {
                    InterlockedCompareExchange64(&_tail, newNode, tail);
                    InterlockedIncrement(&_useSize);
                    break;
                }
            }
        }
    }

    T Dequeue()
    {
        T data = 0;

        for (;;)
        {
            __int64 tail = _tail;
            __int64 head = _head;
            QueueNode* headNode = (QueueNode*)(head & _addressMask);
            __int64 next = headNode->_next;

            if (_useSize == 0) return 0;

            short QID = GetQID(next);
            if (QID != _QID) continue; // Check where tailNode is allocated     
            
            if ((next & _addressMask) == NULL) return 0;

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
                continue;
            }

            QueueNode* dataNode = ((QueueNode*)(next & _addressMask));
            data = dataNode->_data;

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                _pQueuePool->Free(headNode);
                InterlockedDecrement(&_useSize);
                return data;
            }
        }
        return 0;
    }
};


template<typename T>
short CLockFreeQueue<T>::_QIDSupplier = 0;
template<typename T>
unsigned __int64 CLockFreeQueue<T>::_QIDMask = 0b1111111111111111100000000000000000000000000000000000000000000000;
template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pQueuePool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);