#pragma once
#include "CTlsPool.h"

// #define __NODE_DEBUG
// #define __QUEUE_DEBUG

template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
#define __KEY_BIT__ 9
#define __ADDRESS_BIT__ 47
    static short _key;
    unsigned __int64 _QIDMask = 0;
    unsigned __int64 _keyAddressMask = 0;
    unsigned __int64 _addressMask = 0;

public:
    class QueueNode
    {
        friend CLockFreeQueue;

    public:
        QueueNode()
        {
            __debugbreak();
        }

        QueueNode(T data, __int64 next, long QID) : _data(data)
        {
            unsigned __int64 nodeID = (unsigned __int64)QID;
            QID <<= (__KEY_BIT__ + __ADDRESS_BIT__);
            next |= QID;
        }

        ~QueueNode()
        {
            _data = 0;
            _next = 0;
        }

    private:
        T _data = 0;
        __int64 _next = 0;

#ifdef __NODE_DEBUG
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
#define dfNODE_DEBUG_MAX 100
        inline void LeaveLog(int line, int QID, int nodeID)
        {
            LONG idx = InterlockedIncrement(&_nodeDebugIdx);
            _nodeDebugArray[idx % dfNODE_DEBUG_MAX].SetData(GetCurrentThreadId(), line, QID, nodeID);
        }
        NodeDebugData _nodeDebugArray[dfNODE_DEBUG_MAX];
        volatile long _nodeDebugIdx = -1;

#endif
    };

private:
    // 8 bit QID + 9 bit key + 47 bit address
    __int64 CreateAddress(QueueNode* node)
    {
        unsigned __int64 QID = (unsigned __int64) _QID;
        QID <<= (__KEY_BIT__ + __ADDRESS_BIT__);

        unsigned __int64 key = (unsigned __int64) InterlockedIncrement16(&_key);
        key <<= __ADDRESS_BIT__;  
        key |= QID;

        __int64 address = (__int64)node;
        address &= _addressMask;
        address |= key;
        return address;
    }

public:
    CLockFreeQueue()
    { 
        _QID = InterlockedIncrement16(&_IDSupplier);

        _QIDMask = 0b11111111;
        _QIDMask <<= (__KEY_BIT__ + __ADDRESS_BIT__);
        _keyAddressMask = ~_QIDMask;

        _addressMask = 0b11111111111111111;
        _addressMask <<= __ADDRESS_BIT__;
        _addressMask = ~_addressMask;

        QueueNode* node = _pPool->Alloc(777, 0, _QID);
        // node->LeaveLog(0, _QID, (node->_next & _QIDMask));
        _head = CreateAddress(node);
        _tail = _head;
        _size = 0;
    }

public:   
    static volatile short _IDSupplier;
    static CTlsPool<QueueNode>* _pPool;

private:
    __int64 _head;
    __int64 _tail;
    volatile long _size;
    short _QID;

public:
    void Enqueue(T data)
    {
        QueueNode* node = _pPool->Alloc(data, NULL, _QID);
        // node->LeaveLog(0, _QID, (node->_next & _QIDMask));
        __int64 newNode = CreateAddress(node);

        while (true)
        {
            int size = _size;
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask); 
            __int64 next = tailNode->_next;

            if ((next & _keyAddressMask) != NULL)
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

            if ((next & _keyAddressMask) != NULL)
            {
                return data;
            }

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }
            else if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                QueueNode* dataNode = ((QueueNode*)(next & _addressMask));
                data = dataNode->_data;     
                // headNode->LeaveLog(7, _QID, (dataNode->_next & _QIDMask));
                _pPool->Free(headNode);                
                break;
            }
        }

        InterlockedAdd(&_size, -1);
        return data;
    }

public:
    int GetUseSize() { return _size; }


#ifdef __QUEUE_DEBUG
private: // For Log
    class QueueDebugData
    {
        friend CLockFreeQueue;

    private:
        QueueDebugData() : _idx(-1), _threadID(-1), _line(-1), _node(nullptr) {}

        void SetData(int idx, int threadID, int line, QueueNode* node)
        {
            _idx = idx;
            _threadID = threadID;
            _line = line;
            _node = node;
        }

    private:
        int _idx;
        int _threadID;
        int _line;
        QueueNode* _node;
    };

private:
#define dfQUEUE_DEBUG_MAX 100

    inline void LeaveLog(int line, QueueNode* node = nullptr)
    {
        LONG idx = InterlockedIncrement(&_queueDebugIdx);
        // if (idx >= dfQUEUE_DEBUG_MAX) __debugbreak();
        _queueDebugArray[idx % dfQUEUE_DEBUG_MAX].SetData(idx, GetCurrentThreadId(), line, node);
    }

    QueueDebugData _queueDebugArray[dfQUEUE_DEBUG_MAX];
    volatile long _queueDebugIdx = -1;
#endif
};

template<typename T>
short CLockFreeQueue<T>::_key = 0;

template<typename T>
volatile short CLockFreeQueue<T>::_IDSupplier = 0;

template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);