#pragma once
#include "LockFreePool.h"

#define ENQ_CAS1 0
#define ENQ_CAS2 1
#define DEQ_CAS  2

#define ENQ_CAS_FAIL  5
#define ENQ_NEXT_NOT_NULL  6
#define DEQ_CAS_FAIL  5
#define DEQ_NEXT_NULL  6

template<typename DATA>
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
        QueueNode()
        {
            _data = 0;
            _next = 0;
        }

        ~QueueNode()
        {
            _data = 0;
            _next = 0;
        }

    public:
        bool operator != (const QueueNode& other)
        {
            return _data != other._data;
        }

    private:
        DATA _data;
        __int64 _next;
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
    void Enqueue(DATA data)
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
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            DATA tailVal = tailNode->_data;
            __int64 next = tailNode->_next;

            if (next == NULL)
            {
                __int64 result1 = InterlockedCompareExchange64(&tailNode->_next, newNode, next);
                if (result1 == next)
                {
                    int size = InterlockedExchangeAdd(&_size, 1);
                    LeaveLog(ENQ_CAS1, size, (unsigned __int64)newNode, data, (unsigned __int64)next, 0, 0, 0);

                    __int64 result2 = InterlockedCompareExchange64(&_tail, newNode, tail);
                    
                    if (result2 != tail)
                    {
                        size = InterlockedExchangeAdd(&_size, -1);
                        InterlockedCompareExchange64(&tailNode->_next, node->_next, newNode);
                    }

                    LeaveLog(ENQ_CAS2, size, (unsigned __int64)newNode, data, (unsigned __int64)tail, tailVal,
                        (unsigned __int64)result2, ((QueueNode*)(result2 & _addressMask))->_data);
                    
                    break;
                }
                else if (_casFailFlag)
                {
                    LeaveLog(ENQ_CAS_FAIL, -1, (unsigned __int64)newNode, data, (unsigned __int64)next, 0,
                        (unsigned __int64)result1, ((QueueNode*)(result1 & _addressMask))->_data);
                }
            }
            else if (_casFailFlag)
            {
                LeaveLog(ENQ_NEXT_NOT_NULL, -1, (unsigned __int64)newNode, data, 
                    (unsigned __int64)next, ((QueueNode*)(next & _addressMask))->_data, 0, 0);
            }
        }
    }

    int Dequeue()
    {
        if (_size == 0)
            return -1;

        while (true)
        {
            __int64 head = _head;
            QueueNode* headNode = (QueueNode*)(head & _addressMask);
            DATA headVal = headNode->_data;

            __int64 next = headNode->_next;
            QueueNode* nextNode = (QueueNode*)(next & _addressMask);

            if (nextNode != nullptr)
            {
                DATA nextVal = nextNode->_data;
                __int64 result = InterlockedCompareExchange64(&_head, next, head);
                if (result == head)
                {
                    int size = InterlockedExchangeAdd(&_size, -1);
                    LeaveLog(DEQ_CAS, size, (unsigned __int64)next, nextVal, (unsigned __int64)head, headVal, 0, 0);
                    _pPool->Free(headNode);
                    break;
                }
                else if (_casFailFlag)
                {
                    LeaveLog(DEQ_CAS_FAIL, -1, (unsigned __int64)next, nextVal, (unsigned __int64)head, headVal, 
                        (unsigned __int64)result, ((QueueNode*)(result & _addressMask))->_data);
                }
            }
            else
            {
                if (_casFailFlag) LeaveLog(DEQ_NEXT_NULL, -1, (unsigned __int64)next, 0, 
                    (unsigned __int64)head, headVal, 0, 0);
                return -1;
            }
        }

        return 0;
    }

private:
    class QueueDebugData
    {
        friend CLockFreeQueue;

    private:
        QueueDebugData() : _idx(-1), _threadID(-1), _line(-1),
            _compKey(-1), _exchKey(-1), _compAddress(-1), _exchAddress(-1) {}

        void SetData(int idx, int threadID, int line, int size,
            int exchKey, __int64 exchAddress, DATA exchVal,
            int compKey, __int64 compAddress, DATA compVal,
            int realKey, __int64 realAddress, DATA realVal)
        {
            _exchVal = exchVal;
            _exchKey = exchKey;
            _exchAddress = exchAddress;

            _compVal = compVal;
            _compKey = compKey;
            _compAddress = compAddress;

            _realVal = realVal;
            _realKey = realKey;
            _realAddress = realAddress;

            _idx = idx;
            _threadID = threadID;
            _line = line;
            _size = size;

            __faststorefence();
        }

    private:
        int _idx;
        int _threadID;
        int _line;
        int _size;

        int _compKey;
        __int64 _compAddress;
        DATA _compVal;

        int _exchKey;
        __int64 _exchAddress;
        DATA _exchVal;

        int _realKey;
        __int64 _realAddress;
        DATA _realVal;
    };

private:
#define dfQUEUE_DEBUG_MAX 100000
    QueueDebugData _queueDebugArray[dfQUEUE_DEBUG_MAX];
    volatile long _queueDebugIdx = -1;

#define dfSAVE_IDX_MAX 100
    QueueDebugData* _sizeZeroArray[dfSAVE_IDX_MAX];
    QueueDebugData* _casFailArray[dfSAVE_IDX_MAX];
    volatile long _sizeZeroIdx = -1;
    volatile long _casFailIdx = -1;

private:
    bool _casFailFlag = false;

private:
    inline void LeaveLog(int line, int size,
        unsigned __int64 exchange, DATA exchVal,
        unsigned __int64 comperand, DATA compVal,
        unsigned __int64 real, DATA realVal)
    {
        LONG idx = InterlockedIncrement(&_queueDebugIdx);
        if (idx >= dfQUEUE_DEBUG_MAX) __debugbreak();

        int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
        int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
        int realKey = (real >> __USESIZE_64BIT__) & _keyMask;
        __int64 exchAddress = exchange & _addressMask;
        __int64 compAddress = comperand & _addressMask;
        __int64 realAddress = real & _addressMask;

        _queueDebugArray[idx % dfQUEUE_DEBUG_MAX].SetData(
            idx, GetCurrentThreadId(), line, size,
            exchKey, exchAddress, exchVal,
            compKey, compAddress, compVal,
            realKey, realAddress, realVal);

        if (_head == _tail)
        {
            LONG sizeZeroIdx = InterlockedIncrement(&_sizeZeroIdx);
            _sizeZeroArray[sizeZeroIdx % dfSAVE_IDX_MAX] = &_queueDebugArray[idx % dfQUEUE_DEBUG_MAX];

            //::printf("Size %d: %d (%d)\n", size, idx, idx % dfQUEUE_DEBUG_MAX);
        }

        if (line == ENQ_CAS2 && comperand != real)
        {
            LONG casFailIdx = InterlockedIncrement(&_casFailIdx);
            _casFailArray[casFailIdx % dfSAVE_IDX_MAX] = &_queueDebugArray[idx % dfQUEUE_DEBUG_MAX];
            _casFailFlag = true;

            ::printf("CAS2 fail: %d (%d)\n", idx, idx % dfQUEUE_DEBUG_MAX);          
            //__debugbreak();
        }
    }
};

