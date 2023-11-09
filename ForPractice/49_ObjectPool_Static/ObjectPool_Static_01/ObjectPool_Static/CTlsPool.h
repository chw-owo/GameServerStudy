#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <new.h>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#define BUCKET_SIZE 200
#define THREAD_MAX 20

long _bucketID = 0;
DWORD _tlsIdx = 0;

template <typename T>
class CLockFreeQueue;

template <class T>
class CTlsPool
{
public:
	class stBucket
	{
	public:
		stBucket(int ID) : _ID(ID) {};

	public:
		int _ID = 0;
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

	public:
		CTlsPool<T>* _mainPool;
		bool _placementNew;
		stBucket* _bucket;
		int _bucketIdx = BUCKET_SIZE;
		stBucket* _return;
		int _returnIdx = 0;
	};

public:
	template<typename... Types>
	CTlsPool(int blockNum, bool placementNew, Types... args);
	~CTlsPool();

public:
	template<typename... Types>
	inline T* Alloc(int line, int idx, int QID, int nodeID, CLockFreeQueue<int>* Q, int& allocIdx, Types... args);
	template<typename... Types>
	inline void Initialize(Types... args);
	inline void Free(T* pData, int line, int idx, int QID, int nodeID, CLockFreeQueue<int>* Q, int& returnIdx);

public: // For protect ABA
#define __USESIZE_64BIT__ 47
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;
	__int64 _buckets = NULL;

public:
	bool _placementNew;
	int _bucketNum;

public: // Debug
#define dfPOOL_DEBUG_MAX 10000

	class PoolDebugData
	{
	public:
		PoolDebugData() : _line(-1),  _idx(-1), _threadID(-1), _QID(-1), _nodeQID(-1), _address(nullptr) {};

		void SetData(int line, int idx, int threadID, int QID, int nodeQID, T* address, CLockFreeQueue<int>* Q)
		{
			CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
			_bucketIdx = pool->_bucketIdx;
			_returnIdx = pool->_returnIdx;
			_idx = idx;
			_line = line;
			_threadID = threadID;
			_QID = QID;
			_nodeQID = nodeQID;
			_address = address;
			_Q = Q;
		}

	public:
		int _line;
		int _idx;
		int _threadID;
		int _QID;
		int _nodeQID;
		
		int _bucketIdx;
		int _returnIdx;
		T* _address;
		CLockFreeQueue<int>* _Q;
	};

	inline void LeaveLog(int line, int idx, int QID, int nodeQID, T* address, CLockFreeQueue<int>* Q)
	{
		LONG debugIdx = InterlockedIncrement(&_poolDebugIdx);
		_poolDebugArray[debugIdx % dfPOOL_DEBUG_MAX].SetData(line, idx, GetCurrentThreadId(), QID, nodeQID, address, Q);
	}
	PoolDebugData _poolDebugArray[dfPOOL_DEBUG_MAX];
	volatile long _poolDebugIdx = -1;
};

template<class T>
inline CTlsPool<T>::CPool::CPool(CTlsPool<T>* mainPool, bool placementNew) : _mainPool(mainPool), _placementNew(placementNew)
{
	int ret = InterlockedIncrement(&_bucketID);
	_return = new stBucket(ret);
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

		// 초기 할당 시 생성자를 호출하지 않으므로 이때 소멸자를 호출하면 안된다. 
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

		// 초기 할당 시 생성자를 호출했으므로 이때 소멸자를 호출해야 된다. 
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
	key <<= __USESIZE_64BIT__;
	__int64 pNew = (__int64)_return;
	pNew &= _mainPool->_addressMask;
	pNew |= key;

	for (;;)
	{
		__int64 pCur = _mainPool->_buckets;
		_return->_tail = pCur;
		if (InterlockedCompareExchange64(&_mainPool->_buckets, pNew, pCur) == pCur) break;
	}

	int ret = InterlockedIncrement(&_bucketID);
	_return = new stBucket(ret);
	_returnIdx = 0;
}

template<class T>
template<typename ...Types>
inline void CTlsPool<T>::CPool::GetBucket(Types ...args)
{
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
				_bucketIdx = 0;
				return;
			}
		}
	}
}

template<class T>
template<typename ...Types>
inline void CTlsPool<T>::CPool::CreateBucket(Types ...args)
{
	int ret = InterlockedIncrement(&_bucketID);
	_bucket = new stBucket(ret);

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

	_bucketIdx = 0;
}

template<class T>
template<typename ...Types>
inline T* CTlsPool<T>::CPool::Alloc(Types ...args)
{
	if (_bucketIdx == BUCKET_SIZE) GetBucket(args...);
	return _bucket->_datas[_bucketIdx++];
}

template<class T>
inline void CTlsPool<T>::CPool::Free(T* data)
{
	if (_returnIdx == BUCKET_SIZE) ReturnBucket();
	_return->_datas[_returnIdx++] = data;
}

template<class T>
template<typename ...Types>
inline CTlsPool<T>::CTlsPool(int blockNum, bool placementNew, Types ...args)
{
	_tlsIdx = TlsAlloc();

	if (_tlsIdx == TLS_OUT_OF_INDEXES)
	{
		int err = GetLastError();
		::printf("%d\n", err);
		__debugbreak();
	}

	_keyMask = 0b11111111111111111;
	_addressMask = 0b11111111111111111;
	_addressMask <<= __USESIZE_64BIT__;
	_addressMask = ~_addressMask;

	if (blockNum <= 0) return;

	_bucketNum = (blockNum / BUCKET_SIZE) + 1;
	if (_placementNew)
	{
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다
		for (int i = 0; i < _bucketNum; i++)
		{
			int ret = InterlockedIncrement(&_bucketID);
			stBucket* bucket = new stBucket(ret);
			bucket->_tail = (__int64)_buckets;
			for (int j = 0; j < BUCKET_SIZE; j++)
			{
				bucket->_datas[j] = (T*)malloc(sizeof(T));
			}

			__int64 key = (__int64)InterlockedIncrement64(&_key);
			key <<= __USESIZE_64BIT__;
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
			int ret = InterlockedIncrement(&_bucketID);
			stBucket* bucket = new stBucket(ret);
			bucket->_tail = (__int64)_buckets;

			for (int j = 0; j < BUCKET_SIZE; j++)
			{
				bucket->_datas[j] = (T*)malloc(sizeof(T));
				new (bucket->_datas[j]) T(args...);
			}

			__int64 key = (__int64)InterlockedIncrement64(&_key);
			key <<= __USESIZE_64BIT__;
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
inline T* CTlsPool<T>::Alloc(int line, int idx, int QID, int nodeID, CLockFreeQueue<int>* Q, int& allocIdx, Types ...args)
{
	CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
	if (pool == nullptr)
	{
		pool = new CPool(this, _placementNew);
		if (pool == nullptr) __debugbreak();
		bool ret = TlsSetValue(_tlsIdx, (LPVOID)pool);
		if (ret == 0)
		{
			int err = GetLastError();
			printf("%d\n", err);
			__debugbreak();
		}
	}

	T* ret = pool->Alloc(args...);
	allocIdx = pool->_bucket->_ID;
	LeaveLog(line, idx, QID, nodeID, ret, Q);
	return ret;
}

template<class T> 
inline void CTlsPool<T>::Free(T* data, int line, int idx, int QID, int nodeID, CLockFreeQueue<int>* Q, int& returnIdx)
{
	CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
	if (pool == nullptr)
	{
		pool = new CPool(this, _placementNew);
		if (pool == nullptr) __debugbreak();
		bool ret = TlsSetValue(_tlsIdx, (LPVOID)pool);
		if (ret == 0)
		{
			int err = GetLastError();
			printf("%d\n", err);
			__debugbreak();
		}
	}

	pool->Free(data);
	returnIdx = pool->_return->_ID;
	LeaveLog(line, idx, QID, nodeID, data, Q);
}


template<class T>
template<typename ...Types>
inline void CTlsPool<T>::Initialize(Types ...args)
{
	CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
	if (pool == nullptr)
	{
		pool = new CPool(this, _placementNew);
		if (pool == nullptr) __debugbreak();
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

