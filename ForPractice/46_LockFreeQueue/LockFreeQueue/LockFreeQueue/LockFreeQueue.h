#pragma once
#include "LockFreePool.h"
#include "TestData.h"

#define ENQ_CAS1 0
#define ENQ_CAS2 1
#define ENQ_CAS3 2
#define DEQ_CAS  3
#define ENQ_CAS2_FAIL 9999

#define ENQ_CAS3_FAIL 100
#define DEQ_CAS_FAIL 101
#define DEQ_NEXT_NULL 102

class CLockFreeQueue
{
public:
    CLockFreeQueue()
    {
        _pPool = new CLockFreePool<QueueNode>(0, true);
        _keyMask = 0b11111111111111111;
        _addressMask = 0b11111111111111111;
        _addressMask <<= __USESIZE_64BIT__;
        _addressMask = ~_addressMask;

        QueueNode* node = _pPool->Alloc();
        node->_next = NULL;

        //For Protect ABA 
        unsigned __int64 ret = (unsigned __int64)InterlockedIncrement64(&_key);
        unsigned __int64 key = ret;
        key <<= __USESIZE_64BIT__;
        __int64 tmp = (__int64)node;
        tmp &= _addressMask;
        tmp |= key;

        _head = tmp;
        _tail = _head;
        _size = 0;
    }

private:
    class QueueNode
    {
        friend CLockFreeQueue;
    public:
        bool operator != (const QueueNode& other)
        {
            return _data->_value != other._data->_value;
        }

    private:
        TestData* _data = nullptr;
        __int64 _next = 0;
    };

    __int64 _head;
    __int64 _tail;

private:
    long _size;
    CLockFreePool<QueueNode>* _pPool;

private: // For protect ABA
#define __USESIZE_64BIT__ 47
    unsigned __int64 _addressMask = 0;
    unsigned int _keyMask = 0;
    volatile __int64 _key = 0;

public:
    void Enqueue(TestData* data)
    {
        QueueNode* node = _pPool->Alloc();
        node->_data = data;
        node->_next = NULL;

        unsigned __int64 ret = (unsigned __int64)InterlockedIncrement64(&_key);
        unsigned __int64 key = ret;
        key <<= __USESIZE_64BIT__;
        __int64 newNode = (__int64)node;
        newNode &= _addressMask;
        newNode |= key;

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
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {                   
                    InterlockedCompareExchange64(&_tail, newNode, tail);
                    break;
                }
            }           
        }

        InterlockedExchangeAdd(&_size, 1);
    }

    TestData* Dequeue()
    {
        TestData* data = nullptr;
        if (_size == 0) return data;

        while (true)
        {
            int size = _size;
            __int64 tail = _tail;
            __int64 head = _head;
            QueueNode* headNode = (QueueNode*)(head & _addressMask);
            __int64 next = headNode->_next;

            if(next == NULL)
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

private: // For Log
    class QueueDebugData
    {
        friend CLockFreeQueue;

    private:
        QueueDebugData() : _idx(-1), _threadID(-1), _line(-1), _size(-1), _compKey(-1), _exchKey(-1), _realKey(-1), 
            _compAddress(-1), _exchAddress(-1), _realAddress(-1), _compData(nullptr), _exchData(nullptr) {}

        void SetData(int idx, int threadID, int line, int size, int exchKey, __int64 exchAddress, TestData* exchData,
            int compKey, __int64 compAddress, TestData* compData, int realKey, __int64 realAddress)
        {
            _exchKey = exchKey;
            _exchAddress = exchAddress;
            _exchData = exchData;

            _compKey = compKey;
            _compAddress = compAddress;
            _compData = compData;

            _realKey = realKey;
            _realAddress = realAddress;

            _idx = idx;
            _threadID = threadID;
            _line = line;
            _size = size;
        }

    private:
        int _idx;
        int _threadID;
        int _line;
        int _size;

        int _compKey;
        __int64 _compAddress;
        TestData* _compData;

        int _exchKey;
        __int64 _exchAddress;
        TestData* _exchData;

        int _realKey;
        __int64 _realAddress;
    };
    
private: 
#define dfQUEUE_DEBUG_MAX 2000000
#define dfSAVE_IDX_MAX 1000

    inline void LeaveLog(int line, int size, unsigned __int64 exchange, TestData* exchData, 
        unsigned __int64 comperand, TestData* compData, unsigned __int64 real)
    {
        LONG idx = InterlockedIncrement(&_queueDebugIdx);

        int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
        int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
        int realKey = (real >> __USESIZE_64BIT__) & _keyMask;
        __int64 exchAddress = exchange & _addressMask;
        __int64 compAddress = comperand & _addressMask;
        __int64 realAddress = real & _addressMask;

        _queueDebugArray[idx % dfQUEUE_DEBUG_MAX].SetData(idx, GetCurrentThreadId(), line, size,
            exchKey, exchAddress, exchData, compKey, compAddress, compData, realKey, realAddress);
        
        if (line == ENQ_CAS2_FAIL)
        {
            LONG casFailIdx = InterlockedIncrement(&_casFailIdx);
            _casFailArray[casFailIdx % dfSAVE_IDX_MAX] = &_queueDebugArray[idx % dfQUEUE_DEBUG_MAX];
        }
    }

    QueueDebugData _queueDebugArray[dfQUEUE_DEBUG_MAX];
    volatile long _queueDebugIdx = -1;
    QueueDebugData* _casFailArray[dfSAVE_IDX_MAX] = { nullptr, };
    volatile long _casFailIdx = -1;
};

