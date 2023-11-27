#pragma once
#include "CTlsPool.h"

template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
#define __KEY_BIT__ 9
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
        QueueNode(T data, long QID): _data(data)
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
    static CTlsPool<QueueNode>* _pPool;

private:
    __int64 _head;
    __int64 _tail;
    long _size;
    short _QID;

private:
    // 8 bit QID + 9 bit key + 47 bit address
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
        QID &= 0b11111111;
        return QID;
    }

public:
    CLockFreeQueue()
    {
        _QID = InterlockedIncrement16(&_QIDSupplier);

        _QIDMask = 0b11111111;
        _QIDMask <<= (__KEY_BIT__ + __ADDRESS_BIT__);
        _keyAddressMask = ~_QIDMask;

        _addressMask = 0b11111111111111111;
        _addressMask <<= __ADDRESS_BIT__;
        _addressMask = ~_addressMask;

        QueueNode* node = _pPool->Alloc(0, _QID);
        _head = CreateAddress(node);
        _tail = _head;
        _size = 0;
    }

public:
    void Enqueue(T data)
    {
        QueueNode* node = _pPool->Alloc(data, _QID);
        __int64 newNode = CreateAddress(node);

       for(;;)
        {
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;
            if (GetQID(next) != _QID) continue;

            if ((next & _addressMask) != NULL)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }
            else
            {
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {
                    InterlockedCompareExchange64(&_tail, newNode, tail);
                    InterlockedIncrement(&_size);
                    break;
                }
            }
        }
    }

    T Dequeue()
    {
        T data = 0;
        
        for(;;)
        {
            __int64 tail = _tail;
            __int64 head = _head;
            QueueNode* headNode = (QueueNode*)(head & _addressMask);
            __int64 next = headNode->_next;

            if (_size == 0) return 0;
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
                if (data != 0 && data != _QID) __debugbreak();
                _pPool->Free(headNode);
                InterlockedDecrement(&_size);
                return data;
            }
        }
        return 0;
    }
};


template<typename T>
short CLockFreeQueue<T>::_QIDSupplier = 0;
template<typename T>
unsigned __int64 CLockFreeQueue<T>::_QIDMask = 0;
template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);