#pragma once
#include "CTlsPool.h"

template <typename T>
class CLockFreeQueue
{
public:
    class QueueNode
    {
        friend CLockFreeQueue;

    public:
        QueueNode()
        {
            ::printf("No~~~~\n");
            __debugbreak();
        }

        QueueNode(T data, __int64 next, short QID) : _data(data), _next(next), _QID(QID) {};
        ~QueueNode()
        {
            _data = 0;
            _next = 0;
            _QID = 0;
        }

    private:
        T _data = 0;
        __int64 _next = 0;
        short _QID = 0;
    };

private: // For protect ABA
#define __ADDRESS_BIT__ 47
    volatile __int64 _key = 0;
    unsigned __int64 _keyMask = 0;
    unsigned __int64 _addressMask = 0;

public:
    /*
    #define __KEY_BIT__ 8
    // 9 bit Q Idx + 8 bit key + 47 bit address
    __int64 CreateAddress(T* node)
    {
        unsigned __int64 idx = (unsigned __int64)_QID;
        idx <<= (__KEY_BIT__ + __ADDRESS_BIT__);

        short ret = InterlockedIncrement(&_key);
        unsigned __int64 key = (unsigned __int64)ret;
        key <<= __ADDRESS_BIT__;
        key |= idx;

        __int64 address = (__int64)node;
        address &= _addressMask;
        address |= key;
        return address;
    }
    */

    // 17 bit key + 47 bit address
    __int64 CreateAddress(QueueNode* node)
    {
        unsigned __int64 key = (unsigned __int64) InterlockedIncrement64(&_key);
        key <<= __ADDRESS_BIT__;     
        __int64 address = (__int64)node;
        address &= _addressMask;
        address |= key;

        return address;
    }

public:
    CLockFreeQueue()
    { 
        _QID = InterlockedIncrement16(&_IDSupplier);

        _keyMask = 0b11111111111111111;
        _addressMask = 0b11111111111111111;
        _addressMask <<= __ADDRESS_BIT__;
        _addressMask = ~_addressMask;

        QueueNode* node = _pPool->Alloc(0, 0, _QID);
        __int64 address = CreateAddress(node);
        _head = address;
        _tail = _head;
        _size = 0;
    }

public:   
    static short _IDSupplier;
    static CTlsPool<QueueNode>* _pPool;

private:
    __int64 _head;
    __int64 _tail;
    long _size;
    short _QID;

public:
    void Enqueue(T data)
    {
        QueueNode* node = _pPool->Alloc(data, NULL, _QID);
        __int64 newNode = CreateAddress(node);

        while (true)
        {
            int size = _size;
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;

            if (next != NULL)
            {
                InterlockedCompareExchange64(&_tail, next, tail);               
            }
            else
            {
                if (tailNode->_QID != _QID) continue;
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {
                    InterlockedCompareExchange64(&_tail, newNode, tail);
                    break;
                }
            } 
        }

        InterlockedExchangeAdd(&_size, 1);
    }

    T Dequeue()
    {
        T data = 0;
        int size = _size;
        if (size == 0)
        {
            return data;
        }

        while (true)
        {
            size = _size;
            __int64 tail = _tail;
            __int64 head = _head;
            QueueNode* headNode = (QueueNode*)(head & _addressMask);
            __int64 next = headNode->_next;

            if (next == NULL)
            {
                return data;
            }

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                data = ((QueueNode*)(next & _addressMask))->_data;
                _pPool->Free(headNode);
                break;
            }
        }

        InterlockedExchangeAdd(&_size, -1);
        return data;
    }

public:
    int GetUseSize() { return _size; }

private: // For Log
    class QueueDebugData
    {
        friend CLockFreeQueue;

    private:
        QueueDebugData() : _idx(-1), _threadID(-1), _line(-1), _size(-1), _data(-1),
            _compKey(-1), _exchKey(-1), _compAddress(-1), _exchAddress(-1){}

        void SetData(int idx, int threadID, int line, int size, __int64 data,
            int exchKey, __int64 exchAddress, int compKey, __int64 compAddress)
        {
            _idx = idx;
            _threadID = threadID;
            _line = line;
            _size = size;
            _data = data;

            _exchKey = exchKey;
            _exchAddress = exchAddress;
            _compKey = compKey;
            _compAddress = compAddress;
        }

    private:
        int _idx;
        int _threadID;
        int _line;
        int _size;
        __int64 _data;

        int _compKey;
        int _exchKey;
        __int64 _compAddress;
        __int64 _exchAddress;     
    };

private:
#define dfQUEUE_DEBUG_MAX 100

    inline int LeaveLog(int line, int size, unsigned __int64 data,
        unsigned __int64 exchange, unsigned __int64 comperand)
    {
        LONG idx = InterlockedIncrement(&_queueDebugIdx);

        int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
        int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
        __int64 exchAddress = exchange & _addressMask;
        __int64 compAddress = comperand & _addressMask;

        _queueDebugArray[idx % dfQUEUE_DEBUG_MAX].SetData(
            idx, GetCurrentThreadId(), line, size, data,
            exchKey, exchAddress, compKey, compAddress);

        return idx;
    }

    QueueDebugData _queueDebugArray[dfQUEUE_DEBUG_MAX];
    volatile long _queueDebugIdx = -1;
};

template<typename T>
short CLockFreeQueue<T>::_IDSupplier = 0;

template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);