#pragma once
#include "CTlsPool.h"

#define __QUEUE_DEBUG

#ifdef __QUEUE_DEBUG
template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
#define __KEY_BIT__ 9
#define __ADDRESS_BIT__ 47
    static short _key;
    static unsigned __int64 _QIDMask;
    unsigned __int64 _keyAddressMask = 0;
    unsigned __int64 _addressMask = 0;

public:
    class QueueNode
    {
        friend CLockFreeQueue;

    public:
        QueueNode()
        {
            //__debugbreak();
        }

        //QueueNode(T data, __int64 next, long QID)
        void Initailze(T data, long QID)
        {
            _data = data;
            unsigned __int64 nodeID = (unsigned __int64)QID;
            nodeID <<= (__KEY_BIT__ + __ADDRESS_BIT__);
            nodeID &= _QIDMask;
            _next = nodeID;
        }

        ~QueueNode()
        {
            _data = 0;
            _next = 0;
        }

        void Terminate()
        {
            _data = 0;
            _next = 0;
        }

    private:
        T _data = 0;
        __int64 _next = 0;

    private: // For Log
        class NodeDebugData
        {
        public:
            NodeDebugData() : _threadID(-1), _line(-1), _QID(-1), _nodeID(-1), _QIdx(-1), _front(nullptr) {}

            void SetData(int QIdx, int threadID, int line, int QID, __int64 next, QueueNode* front)
            {
                _threadID = threadID;
                _line = line;
                _QID = QID;
                _nodeID = (unsigned __int64)next >> (__KEY_BIT__ + __ADDRESS_BIT__);
                _nodeID &= 0b11111111;
                _QIdx = QIdx;
                _front = front;
            }

        private:
            int _threadID;
            int _line;
            int _QID;
            unsigned __int64 _nodeID;
            int _QIdx;
            QueueNode* _front;
        };

    private:
#define dfNODE_DEBUG_MAX 30
        inline void LeaveLog(int QIdx, int line, int QID, __int64 next, QueueNode* front = nullptr)
        {
            LONG idx = InterlockedIncrement(&_nodeDebugIdx);
            _nodeDebugArray[idx % dfNODE_DEBUG_MAX].SetData(QIdx, GetCurrentThreadId(), line, QID, next, front);
        }
        NodeDebugData _nodeDebugArray[dfNODE_DEBUG_MAX];
        volatile long _nodeDebugIdx = -1;
    };

private:
    // 8 bit QID + 9 bit key + 47 bit address
    __int64 CreateAddress(QueueNode* node)
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

        QueueNode* node = _pPool->Alloc(); //(0, _QID);
        node->Initailze(0, _QID);
        node->LeaveLog(_queueDebugIdx, 777, _QID, node->_next);
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
        QueueNode* node = _pPool->Alloc(); //(data, _QID);
        node->Initailze(data, _QID);
        node->LeaveLog(_queueDebugIdx, 0, _QID, node->_next);
        __int64 newNode = CreateAddress(node);

        while (true)
        {
            int size = _size;
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;

            if ((next & _keyAddressMask) != NULL)
            {
                if (InterlockedCompareExchange64(&_tail, next, tail) == tail)
                {
                    int ret = LeaveLog(1, tailNode, (QueueNode*)(next & _addressMask));
                    node->LeaveLog(ret, 1, _QID, tailNode->_next, tailNode);
                }
            }
            else
            {
                if (InterlockedCompareExchange64(&tailNode->_next, newNode, next) == next)
                {
                    int ret = LeaveLog(2, (QueueNode*)(next & _addressMask), node);
                    node->LeaveLog(ret, 2, _QID, tailNode->_next, tailNode);
                    if (InterlockedCompareExchange64(&_tail, newNode, tail) == tail)
                    {
                        int ret = LeaveLog(3, tailNode, node);
                        node->LeaveLog(ret, 3, _QID, tailNode->_next, tailNode);
                    }
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

            if ((next & _keyAddressMask) == NULL)
            {
                return data;
            }

            if (head == tail)
            {
                int ret = LeaveLog(6, headNode, (QueueNode*)(next & _addressMask));
                headNode->LeaveLog(ret, 6, _QID, headNode->_next, headNode);
                InterlockedCompareExchange64(&_tail, next, tail);
                continue;
            }

            QueueNode* dataNode = ((QueueNode*)(next & _addressMask));
            data = dataNode->_data;
            __int64 dataNext = dataNode->_next;
            __int64 headNext = headNode->_next;

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                int ret = LeaveLog(8, headNode, dataNode);
                dataNode->LeaveLog(ret, 8, _QID, dataNext, headNode);
                headNode->LeaveLog(ret, 7, _QID, headNext);
                if (_QID != data) __debugbreak();

                headNode->Terminate();
                _pPool->Free(headNode);
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
        QueueDebugData() : _idx(-1), _threadID(-1), _line(-1), _node1(nullptr), _node2(nullptr) {}

        void SetData(int idx, int threadID, int line, QueueNode* node1, QueueNode* node2)
        {
            _idx = idx;
            _threadID = threadID;
            _line = line;
            _node1 = node1;
            _node2 = node2;
        }

    private:
        int _idx;
        int _threadID;
        int _line;
        QueueNode* _node1;
        QueueNode* _node2;
    };

private:
#define dfQUEUE_DEBUG_MAX 5000
    inline int LeaveLog(int line, QueueNode* node1, QueueNode* node2)
    {
        LONG idx = InterlockedIncrement(&_queueDebugIdx);
        // if (idx >= dfQUEUE_DEBUG_MAX) __debugbreak();
        _queueDebugArray[idx % dfQUEUE_DEBUG_MAX].SetData(idx, GetCurrentThreadId(), line, node1, node2);
        return idx;
    }

    QueueDebugData _queueDebugArray[dfQUEUE_DEBUG_MAX];
    volatile long _queueDebugIdx = -1;

};
#else
template <typename T>
class CLockFreeQueue
{
private: // For protect ABA
#define __KEY_BIT__ 9
#define __ADDRESS_BIT__ 47
    static short _key;
    static unsigned __int64 _QIDMask;
    unsigned __int64 _keyAddressMask = 0;
    unsigned __int64 _addressMask = 0;

public:
    class QueueNode
    {
        friend CLockFreeQueue;

    public:
        QueueNode()
        {
            //__debugbreak();
        }

        //QueueNode(T data, __int64 next, long QID)
        void Initailze(T data, long QID)
        {
            _data = data;
            unsigned __int64 nodeID = (unsigned __int64)QID;
            nodeID <<= (__KEY_BIT__ + __ADDRESS_BIT__);
            nodeID &= _QIDMask;
            _next = nodeID;
        }

        ~QueueNode()
        {
            _data = 0;
            _next = 0;
        }

        void Terminate()
        {
            _data = 0;
            _next = 0;
        }

    private:
        T _data = 0;
        __int64 _next = 0;

    private: // For Log
        class NodeDebugData
        {
        public:
            NodeDebugData() : _threadID(-1), _line(-1), _QID(-1), _nodeID(-1), _QIdx(-1), _front(nullptr) {}

            void SetData(int QIdx, int threadID, int line, int QID, __int64 next, QueueNode* front)
            {
                _threadID = threadID;
                _line = line;
                _QID = QID;
                _nodeID = (unsigned __int64)next >> (__KEY_BIT__ + __ADDRESS_BIT__);
                _nodeID &= 0b11111111;
                _QIdx = QIdx;
                _front = front;
            }

        private:
            int _threadID;
            int _line;
            int _QID;
            unsigned __int64 _nodeID;
            int _QIdx;
            QueueNode* _front;
        };

    private:
#define dfNODE_DEBUG_MAX 100
        inline void LeaveLog(int QIdx, int line, int QID, __int64 next, QueueNode* front = nullptr)
        {
            LONG idx = InterlockedIncrement(&_nodeDebugIdx);
            _nodeDebugArray[idx % dfNODE_DEBUG_MAX].SetData(QIdx, GetCurrentThreadId(), line, QID, next, front);
        }
        NodeDebugData _nodeDebugArray[dfNODE_DEBUG_MAX];
        volatile long _nodeDebugIdx = -1;
    };

private:
    // 8 bit QID + 9 bit key + 47 bit address
    __int64 CreateAddress(QueueNode* node)
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

        QueueNode* node = _pPool->Alloc(); //(0, _QID);
        node->Initailze(0, _QID);
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
        QueueNode* node = _pPool->Alloc(); //(data, _QID);
        node->Initailze(data, _QID);
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

            if ((next & _keyAddressMask) == NULL)
            {
                return data;
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
                headNode->Terminate();
                _pPool->Free(headNode);
                break;
            }
        }

        InterlockedAdd(&_size, -1);
        return data;
    }

public:
    int GetUseSize() { return _size; }
};
#endif

template<typename T>
short CLockFreeQueue<T>::_key = 0;

template<typename T>
unsigned __int64 CLockFreeQueue<T>::_QIDMask = 0;

template<typename T>
volatile short CLockFreeQueue<T>::_IDSupplier = 0;

template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, false);