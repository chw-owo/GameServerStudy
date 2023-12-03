#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

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
		int _bucketIdx = BUCKET_SIZE + 1;
		stBucket* _return = nullptr;
		int _returnIdx = 0;
	};

public:
	// TO-DO
	long GetPoolSize()
	{
		return 0;
	}

	long GetNodeCount()
	{
		return 0;
	}

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
	long _curBucketCnt = 0;
	DWORD _tlsIdx = 0;

private:
	CPool* _pools[THREAD_MAX] = { nullptr, };
	long _poolIdx = -1;
};

template<class T>
inline CTlsPool<T>::CPool::CPool(CTlsPool<T>* mainPool, bool placementNew)
	: _mainPool(mainPool), _placementNew(placementNew)
{
	_return = new stBucket;
}

template<class T>
inline CTlsPool<T>::CPool::~CPool()
{
	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵȴ�
		for (int i = 0; i < _returnIdx; i++)
		{
			T* data = _return->_datas[i];
			free(data);
		}

		if (_bucketIdx <= BUCKET_SIZE);
		{
			for (int i = 0; i < _bucketIdx; i++)
			{
				T* data = _bucket->_datas[i];
				free(data);
			}
		}
	}
	else
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		for (int i = 0; i < _returnIdx; i++)
		{
			T* data = _return->_datas[i];
			data->~T();
			free(data);
		}

		if (_bucketIdx <= BUCKET_SIZE);
		{
			for (int i = 0; i < _bucketIdx; i++)
			{
				T* data = _bucket->_datas[i];
				data->~T();
				free(data);
			}
		}
	}
}

template<class T>
inline void CTlsPool<T>::CPool::ReturnBucket()
{
	__int64 key = InterlockedIncrement64(&_mainPool->_key);
	key <<= __ADDRESS_BIT__;
	__int64 pNew = (__int64)_return;
	pNew &= _mainPool->_addressMask;
	pNew |= key;

	for (;;)
	{
		__int64 pCur = _mainPool->_buckets;
		_return->_tail = pCur;
		if (InterlockedCompareExchange64(&_mainPool->_buckets, pNew, pCur) == pCur)
		{
			_return = new stBucket;
			_returnIdx = 0;
			return;
		}
	}
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
			if (bucket == nullptr) __debugbreak();
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
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�
		for (int i = 0; i < BUCKET_SIZE; i++)
		{
			_bucket->_datas[i] = (T*)malloc(sizeof(T));
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
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
	_curBucketCnt = _bucketNum;

	if (_placementNew)
	{
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�
		for (int i = 0; i < _bucketNum; i++)
		{
			stBucket* bucket = (stBucket*)malloc(sizeof(stBucket));
			if (bucket == nullptr) __debugbreak();
			bucket->_tail = (__int64)_buckets;
			for (int j = 0; j < BUCKET_SIZE; j++)
			{
				bucket->_datas[j] = (T*)malloc(sizeof(T));
			}

			__int64 key = InterlockedIncrement64(&_key);
			key <<= __ADDRESS_BIT__;
			__int64 pBuckets = (__int64)bucket;
			pBuckets &= _addressMask;
			pBuckets |= key;

			_buckets = pBuckets;
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		for (int i = 0; i < _bucketNum; i++)
		{
			stBucket* bucket = (stBucket*)malloc(sizeof(stBucket));
			if (bucket == nullptr) __debugbreak();
			bucket->_tail = (__int64)_buckets;
			for (int j = 0; j < BUCKET_SIZE; j++)
			{
				bucket->_datas[j] = (T*)malloc(sizeof(T));
				new (bucket->_datas[j]) T(args...);
			}

			__int64 key = InterlockedIncrement64(&_key);
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
	for (int i = 0; i < _poolIdx; i++)
		delete(_pools[i]);
	TlsFree(_tlsIdx);

	if (_buckets == NULL) return;

	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵȴ�
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
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
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
}

template<class T>
template<typename ...Types>
inline T* CTlsPool<T>::Alloc(Types ...args)
{
	CPool* pool = (CPool*)TlsGetValue(_tlsIdx);
	if (pool == nullptr)
	{
		pool = new CPool(this, _placementNew);
		if (pool == nullptr) __debugbreak();
		long idx = InterlockedIncrement(&_poolIdx);
		_pools[idx] = pool;
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
		if (pool == nullptr) __debugbreak();
		long idx = InterlockedIncrement(&_poolIdx);
		_pools[idx] = pool;
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
		if (pool == nullptr) __debugbreak();
		long idx = InterlockedIncrement(&_poolIdx);
		_pools[idx] = pool;
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
