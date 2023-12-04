#pragma once
#include "CTlsPool.h"
using namespace std;

template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
#define __QUEUE_MAX__ 131071
#define __ADDRESS_BIT__ 47
    long _nodeID = 0;
    static unsigned __int64 _keyMask;
    unsigned __int64 _addressMask = 0;

public:
    class QueueNode
    {
        friend CLockFreeQueue;

    public:
        QueueNode() { __debugbreak(); }
        QueueNode(long QID)
        {
            _next = (__int64)QID;
            _next <<= __ADDRESS_BIT__;
            _next &= CLockFreeQueue::_keyMask;
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
    long _QID;
    static long _QIDSupplier;
    static CTlsPool<QueueNode>* _pQueuePool;

private:
    __int64 _head;
    __int64 _tail;
    long _useSize;

private:
    inline __int64 CreateAddress(QueueNode* node)
    {
        __int64 address = (__int64)node;
        address &= _addressMask;

        unsigned __int64 nodeID = (unsigned __int64)InterlockedIncrement(&_nodeID);
        nodeID <<= __ADDRESS_BIT__;
        nodeID &= CLockFreeQueue::_keyMask;
        address |= nodeID;

        return address;
    }

    inline long GetQID(__int64 next)
    {
        long QID = (((unsigned __int64)next) >> __ADDRESS_BIT__);
        QID &= 0b11111111111111111;
        return QID;
    }

public:
    long GetUseSize() { return _useSize; }

public:
    CLockFreeQueue()
    {
        _QID = InterlockedIncrement(&_QIDSupplier);
        if (_QID > __QUEUE_MAX__) __debugbreak();
        QueueNode* node = _pQueuePool->Alloc(_QID);

        _addressMask = ~CLockFreeQueue::_keyMask;
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

            if ((next & _addressMask) != NULL)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }
            else
            {
                long QID = GetQID(next);
                if (QID != _QID) continue; // Check where tailNode is allocated

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

            if (_useSize == 0)
            {
                return 0;
            }

            if ((next & _addressMask) == NULL)
            {
                long QID = GetQID(next);
                if (QID != _QID) continue; // Check where tailNode is allocated
                return 0;
            }

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
long CLockFreeQueue<T>::_QIDSupplier = 0;
template<typename T>
unsigned __int64 CLockFreeQueue<T>::_keyMask = 0b1111111111111111100000000000000000000000000000000000000000000000;
template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pQueuePool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);