#pragma once

#ifdef tmp

// 1. private pool
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

    ~CLockFreeQueue()
    {
        ::printf("Get: %d\nReturn: %d\nCreate: %d\nAlloc: %d\nFree: %d\n\n",
            _pPool->_getCnt, _pPool->_returnCnt, _pPool->_curBucketCnt, _pPool->_allocCnt, _pPool->_freeCnt);
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

// 2. static pool
// 1) 메모리 계속 증가~~
// 2) DeQ 문제도 해결 안됨~~

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
        QueueNode(T data, long QID) : _data(data)
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

        while (true)
        {
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;
            if (GetQID(next) != _QID) continue;

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
            if ((next & _keyAddressMask) == NULL) return data;

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }

            QueueNode* dataNode = ((QueueNode*)(next & _addressMask));
            data = dataNode->_data;

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                _pPool->Free(headNode);
                break;
            }
        }

        // if (data != 0 && data != _QID) __debugbreak();

        InterlockedDecrement(&_size);
        return data;
    }
};


template<typename T>
short CLockFreeQueue<T>::_QIDSupplier = 0;
template<typename T>
unsigned __int64 CLockFreeQueue<T>::_QIDMask = 0;
template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);

// ver3 Log
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
        QueueNode(T data, long QID) : _data(data)
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

        while (true)
        {
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;
            // if (GetQID(next) != _QID) continue;

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
            if ((next & _keyAddressMask) == NULL) return data;

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }

            QueueNode* dataNode = ((QueueNode*)(next & _addressMask));
            data = dataNode->_data;

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                // if (data != 0 && data != _QID) __debugbreak();
                _pPool->Free(headNode);
                break;
            }
        }

        InterlockedDecrement(&_size);
        return data;
    }
};


template<typename T>
short CLockFreeQueue<T>::_QIDSupplier = 0;
template<typename T>
unsigned __int64 CLockFreeQueue<T>::_QIDMask = 0;
template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);

#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

// #define __DEBUG_POOL

#include <new.h>
#include <stdlib.h>
#include <windows.h>
#define BUCKET_SIZE 200
#define THREAD_MAX 20

template <class T>
class CTlsPool
{
public:
    struct stBucket
    {
        T* _datas[BUCKET_SIZE] = { nullptr, };
        __int64 _tail = NULL;
    };

public:
    class CPool
    {
    public:
        CPool(CTlsPool<T>* mainPool, bool placementNew);
        ~CPool();

    public:
        template<typename... Types>
        inline T* Alloc(Types... args);
        inline void Free(T* pData);

    public:
        inline void ReturnBucket();
        template<typename... Types>
        inline void GetBucket(Types ...args);
        template<typename... Types>
        inline void CreateBucket(Types ...args);

    private:
        CTlsPool<T>* _mainPool;
        bool _placementNew;
        stBucket* _bucket = nullptr;
        int _bucketIdx = BUCKET_SIZE;
        stBucket* _return = nullptr;
        int _returnIdx = 0;

#ifdef __DEBUG_POOL
    private: // For Debug
#define dfDEBUG_MAX 200
        int _threadID = 0;
        int _debugArray[dfDEBUG_MAX] = { 0, };
        long _debugIdx = -1;
        inline void LeaveLog(int line)
        {
            long idx = InterlockedIncrement(&_debugIdx);
            _debugArray[idx % dfDEBUG_MAX] = line;
        }
#endif
    };

#ifdef __DEBUG_POOL
private: // For Debug
    CPool* _pools[THREAD_CNT + 1] = { nullptr, };
    long _poolIdx = -1;
#endif

public:
    template<typename... Types>
    CTlsPool(int blockNum, bool placementNew, Types... args);
    ~CTlsPool();

public:
    template<typename... Types>
    inline T* Alloc(Types... args);
    template<typename... Types>
    inline void Initialize(Types... args);
    inline void Free(T* pData);

public: // For protect ABA
#define __ADDRESS_BIT__ 47
    unsigned __int64 _addressMask = 0;
    unsigned int _keyMask = 0;
    volatile __int64 _key = 0;
    __int64 _buckets = NULL;

public:
    bool _placementNew;
    int _bucketNum;
    DWORD _tlsIdx = 0;
};

template<class T>
inline CTlsPool<T>::CPool::CPool(CTlsPool<T>* mainPool, bool placementNew)
    : _mainPool(mainPool), _placementNew(placementNew)
{
    _return = new stBucket;
#ifdef __DEBUG_POOL 
    _threadID = GetCurrentThreadId();
#endif 

}

template<class T>
inline CTlsPool<T>::CPool::~CPool()
{
    if (_placementNew)
    {
        // Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
        for (int i = 0; i < _returnIdx; i++)
        {
            T* data = _return->_datas[i];
            free(data);
        }

        for (int i = 0; i < _bucketIdx; i++)
        {
            T* data = _bucket->_datas[i];
            free(data);
        }
    }
    else
    {
        // Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다
        for (int i = 0; i < _returnIdx; i++)
        {
            T* data = _return->_datas[i];
            data->~T();
            free(data);
        }

        for (int i = 0; i < _bucketIdx; i++)
        {
            T* data = _bucket->_datas[i];
            data->~T();
            free(data);
        }
    }
}

template<class T>
inline void CTlsPool<T>::CPool::ReturnBucket()
{
    unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_mainPool->_key);
    key <<= __ADDRESS_BIT__;
    __int64 pNew = (__int64)_return;
    pNew &= _mainPool->_addressMask;
    pNew |= key;

    for (;;)
    {
        __int64 pCur = _mainPool->_buckets;
        _return->_tail = pCur;
        if (InterlockedCompareExchange64(&_mainPool->_buckets, pNew, pCur) == pCur) break;
    }

    _return = new stBucket;
    _returnIdx = 0;
}

template<class T>
template<typename ...Types>
inline void CTlsPool<T>::CPool::GetBucket(Types ...args)
{
    _bucketIdx = 0;
    if (_bucket != nullptr) free(_bucket);

    for (;;)
    {
        __int64 pCur = _mainPool->_buckets;
        if (pCur == NULL)
        {
            CreateBucket(args...);
            return;
        }
        else
        {
            stBucket* bucket = (stBucket*)(pCur & _mainPool->_addressMask);
            __int64 pNext = bucket->_tail;
            if (InterlockedCompareExchange64(&_mainPool->_buckets, pNext, pCur) == pCur)
            {
                _bucket = bucket;
                return;
            }
        }
    }
}

template<class T>
template<typename ...Types>
inline void CTlsPool<T>::CPool::CreateBucket(Types ...args)
{
    _bucket = new stBucket;
    if (_placementNew)
    {
        // Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다
        for (int i = 0; i < BUCKET_SIZE; i++)
        {
            _bucket->_datas[i] = (T*)malloc(sizeof(T));
        }
    }
    else
    {
        // Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다
        for (int i = 0; i < BUCKET_SIZE; i++)
        {
            _bucket->_datas[i] = (T*)malloc(sizeof(T));
            new (_bucket->_datas[i]) T(args...);
        }
    }
}

template<class T>
template<typename ...Types>
inline T* CTlsPool<T>::CPool::Alloc(Types ...args)
{
    if (_bucketIdx >= BUCKET_SIZE) GetBucket(args...);
    T* data = _bucket->_datas[_bucketIdx++];
    if (_placementNew) new (data) T(args...);
    return data;
}

template<class T>
inline void CTlsPool<T>::CPool::Free(T* data)
{
    if (_returnIdx >= BUCKET_SIZE) ReturnBucket();
    if (_placementNew) data->~T();
    _return->_datas[_returnIdx++] = data;
}

template<class T>
template<typename ...Types>
inline CTlsPool<T>::CTlsPool(int blockNum, bool placementNew, Types ...args) : _placementNew(placementNew)
{
    _tlsIdx = TlsAlloc();

    _keyMask = 0b11111111111111111;
    _addressMask = 0b11111111111111111;
    _addressMask <<= __ADDRESS_BIT__;
    _addressMask = ~_addressMask;

    if (blockNum <= 0) return;

    _bucketNum = (blockNum / BUCKET_SIZE) + 1;
    if (_placementNew)
    {
        // Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다
        for (int i = 0; i < _bucketNum; i++)
        {
            stBucket* bucket = (stBucket*)malloc(sizeof(stBucket));
            bucket->_tail = (__int64)_buckets;
            for (int j = 0; j < BUCKET_SIZE; j++)
            {
                bucket->_datas[j] = (T*)malloc(sizeof(T));
            }

            __int64 key = (__int64)InterlockedIncrement64(&_key);
            key <<= __ADDRESS_BIT__;
            __int64 pBuckets = (__int64)bucket;
            pBuckets &= _addressMask;
            pBuckets |= key;

            _buckets = pBuckets;
        }
    }
    else
    {
        // Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다
        for (int i = 0; i < _bucketNum; i++)
        {
            stBucket* bucket = (stBucket*)malloc(sizeof(stBucket));
            bucket->_tail = (__int64)_buckets;
            for (int j = 0; j < BUCKET_SIZE; j++)
            {
                bucket->_datas[j] = (T*)malloc(sizeof(T));
                new (bucket->_datas[j]) T(args...);
            }

            __int64 key = (__int64)InterlockedIncrement64(&_key);
            key <<= __ADDRESS_BIT__;
            __int64 pBuckets = (__int64)bucket;
            pBuckets &= _addressMask;
            pBuckets |= key;

            _buckets = (__int64)bucket;
        }
    }
}

template<class T>
inline CTlsPool<T>::~CTlsPool()
{
    if (_buckets == NULL) return;

    if (_placementNew)
    {
        // Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
        stBucket* bucket = (stBucket*)(_buckets & _addressMask);
        while (bucket->_tail != NULL)
        {
            __int64 next = bucket->_tail;
            for (int i = 0; i < BUCKET_SIZE; i++)
            {
                free(bucket->_datas[i]);
            }
            free(bucket);
            bucket = (stBucket*)(next & _addressMask);
        }
        free(bucket);
    }
    else
    {
        // Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다
        stBucket* bucket = (stBucket*)(_buckets & _addressMask);
        while (bucket->_tail != NULL)
        {
            __int64 next = bucket->_tail;
            for (int i = 0; i < BUCKET_SIZE; i++)
            {
                (bucket->_datas[i])->~T();
                free(bucket->_datas[i]);
            }
            free(bucket);
            bucket = (stBucket*)(next & _addressMask);
        }

        for (int i = 0; i < BUCKET_SIZE; i++)
        {
            (bucket->_datas[i])->~T();
            free(bucket->_datas[i]);
        }
        free(bucket);
    }

    TlsFree(_tlsIdx);
}

template<class T>
template<typename ...Types>
inline T* CTlsPool<T>::Alloc(Types ...args)
{
    CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
    if (pool == nullptr)
    {
        pool = new CPool(this, _placementNew);
#ifdef __DEBUG_POOL 
        long idx = InterlockedIncrement(&_poolIdx);
        _pools[idx] = pool;
#endif 

        if (pool == nullptr) __debugbreak();
        bool ret = TlsSetValue(_tlsIdx, (LPVOID)pool);
        if (ret == 0)
        {
            int err = GetLastError();
            ::printf("%d\n", err);
            __debugbreak();
        }
    }
    return pool->Alloc(args...);
}

template<class T>
template<typename ...Types>
inline void CTlsPool<T>::Initialize(Types ...args)
{
    CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
    if (pool == nullptr)
    {
        pool = new CPool(this, _placementNew);
#ifdef __DEBUG_POOL 
        long idx = InterlockedIncrement(&_poolIdx);
        _pools[idx] = pool;
#endif 

        bool ret = TlsSetValue(_tlsIdx, (LPVOID)pool);
        if (ret == 0)
        {
            int err = GetLastError();
            ::printf("%d\n", err);
            __debugbreak();
        }
    }
    pool->GetBucket(args...);
}

template<class T>
inline void CTlsPool<T>::Free(T* data)
{
    CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
    if (pool == nullptr)
    {
        pool = new CPool(this, _placementNew);

#ifdef __DEBUG_POOL 
        long idx = InterlockedIncrement(&_poolIdx);
        _pools[idx] = pool;
#endif 
        bool ret = TlsSetValue(_tlsIdx, (LPVOID)pool);
        if (ret == 0)
        {
            int err = GetLastError();
            ::printf("%d\n", err);
            __debugbreak();
        }
    }
    pool->Free(data);
}

// ver 5
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
        QueueNode(T data, long QID) : _data(data)
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

        while (true)
        {
            __int64 tail = _tail;
            QueueNode* tailNode = (QueueNode*)(tail & _addressMask);
            __int64 next = tailNode->_next;
            if (GetQID(next) != _QID) continue;

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
            if ((next & _keyAddressMask) == NULL) return data;

            if (head == tail)
            {
                InterlockedCompareExchange64(&_tail, next, tail);
            }

            QueueNode* dataNode = ((QueueNode*)(next & _addressMask));
            data = dataNode->_data;

            if (InterlockedCompareExchange64(&_head, next, head) == head)
            {
                if (data != 0 && data != _QID) __debugbreak();
                _pPool->Free(headNode);
                break;
            }
        }

        InterlockedDecrement(&_size);
        return data;
    }
};


template<typename T>
short CLockFreeQueue<T>::_QIDSupplier = 0;
template<typename T>
unsigned __int64 CLockFreeQueue<T>::_QIDMask = 0;
template<typename T>
CTlsPool<typename CLockFreeQueue<T>::QueueNode>* CLockFreeQueue<T>::_pPool = new CTlsPool<CLockFreeQueue<T>::QueueNode>(0, true);
#endif