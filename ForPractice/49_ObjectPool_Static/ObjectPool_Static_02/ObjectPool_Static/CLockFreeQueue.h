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
        QueueNode() { __debugbreak(); }
        QueueNode(T data, __int64 next, long QID) : _data(data), _next(next), _nodeID(QID) 
        {
            LeaveLog(999, QID, _nodeID);
        }
        ~QueueNode()
        {
            _data = 0;
            _next = 0;
            _nodeID = -999;
        }

    private:
        T _data = 0;
        __int64 _next = 0;
        long _nodeID = 0;

    private: // For Log
        class NodeDebugData
        {
        public:
            NodeDebugData() : _threadID(-1), _line(-1), _QID(-1), _nodeID(-1){}

            void SetData(int threadID, int line, int QID, int nodeID)
            {
                _threadID = threadID;
                _line = line;
                _QID = QID;
                _nodeID = nodeID;
            }

        private:
            int _threadID;
            int _line;
            int _QID;
            int _nodeID;
        };

    private:
#define dfNODE_DEBUG_MAX 10000
        inline void LeaveLog(int line, int QID, int nodeID)
        {
            LONG idx = InterlockedIncrement(&_nodeDebugIdx);
            _nodeDebugArray[idx % dfNODE_DEBUG_MAX].SetData(GetCurrentThreadId(), line, QID, nodeID);
        }
        NodeDebugData _nodeDebugArray[dfNODE_DEBUG_MAX];
        volatile long _nodeDebugIdx = -1;
    };

private: // For protect ABA
#define __ADDRESS_BIT__ 47
    static __int64 _key;
    unsigned __int64 _keyMask = 0;
    unsigned __int64 _addressMask = 0;

private:
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
        _QID = _InterlockedIncrement(&_IDSupplier);

        _keyMask = 0b11111111111111111;
        _addressMask = 0b11111111111111111;
        _addressMask <<= __ADDRESS_BIT__;
        _addressMask = ~_addressMask;

        QueueNode* node = _pPool->Alloc(0, 0, _QID);
        node->LeaveLog(0, _QID, node->_nodeID);        
        _head = CreateAddress(node);
        _tail = _head;
        _size = 0;
    }

public:   
    static volatile long _IDSupplier;
    static CTlsPool<QueueNode>* _pPool;

private:
    __int64 _head;
    __int64 _tail;
    volatile long _size;
    long _QID;

public:
    void Enqueue(T data)
    {
        QueueNode* node = _pPool->Alloc(data, NULL, _QID);
        node->LeaveLog(1, _QID, node->_nodeID);
        __int64 newNode = CreateAddress(node);

        while (true)
        {
            int size = _size;
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask); 
            __int64 next = tailNode->_next;

            if (tailNode->_nodeID != _QID)
            {
                tailNode->LeaveLog(9999999999, _QID, tailNode->_nodeID);
                LeaveLog(2, tailNode);
                continue;
            }
            else if (next != NULL)
            {
                InterlockedCompareExchange64(&_tail, next, tail); 
                // LeaveLog(1);
            }
            else
            {
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {
                    // LeaveLog(3);
                    InterlockedCompareExchange64(&_tail, newNode, tail);
                    // LeaveLog(4);
                    break;
                }
            } 
        }

        InterlockedAdd(&_size, 1);
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
                // LeaveLog(5);
            }

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                data = ((QueueNode*)(next & _addressMask))->_data;
                headNode->LeaveLog(6, _QID, headNode->_nodeID);
                _pPool->Free(headNode);
                // LeaveLog(6);
                break;
            }
        }

        InterlockedAdd(&_size, -1);
        return data;
    }

public:
    int GetUseSize() { return _size; }

private: // For Log
    class QueueDebugData
    {
        friend CLockFreeQueue;

    private:
        QueueDebugData() : _threadID(-1), _line(-1), _node(nullptr) {}

        void SetData(int threadID, int line, QueueNode* node)
        {
            _threadID = threadID;
            _line = line;
            _node = node;
        }

    private:
        int _threadID;
        int _line;
        QueueNode* _node;
    };

private:
#define dfQUEUE_DEBUG_MAX 30

    inline void LeaveLog(int line, QueueNode* node = nullptr)
    {
        LONG idx = InterlockedIncrement(&_queueDebugIdx);
        if (idx >= dfQUEUE_DEBUG_MAX) __debugbreak();
        _queueDebugArray[idx % dfQUEUE_DEBUG_MAX].SetData(GetCurrentThreadId(), line, node);
    }

    QueueDebugData _queueDebugArray[dfQUEUE_DEBUG_MAX];
    volatile long _queueDebugIdx = -1;
};

template<typename T>
__int64 CLockFreeQueue<T>::_key = 0;

template<typename T>
volatile long CLockFreeQueue<T>::_IDSupplier = 0;

template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);