#pragma once
#ifndef  __OBJECT_POOL__
#define  __OBJECT_POOL__
#include <new.h>
#include <stdlib.h>
//#define __OBJECT_POOL_DEBUG__

/* ===============================================

<< __OBJECT_POOL_DEBUG__ ���� >>

1. Node

head�� tail�� ���� �� ������� ���� ���� ����.
head, tail�� �Ҵ�� ������ ������ ���� ���̹Ƿ�
Alloc���� ���� �������� Free���� Ȯ���Ѵ�.

�� ��尡 �ƴ� ���� tail�� ���� node�� �ּҰ��� ����.

1) head

32bit�� ���, ���� 1byte�� Object pool ID��
���� 3byte�� Data �ּ��� ���� 3byte�� ������.
64bit�� ���, ���� 4byte�� 0x0000��,
���� 4byte�� 32bit������ head ���� ������.

Object pool ID�� unsigned char(0~255)�̸�
�� �̻��� Object pool�� ��������� ���� ������� �ʾҴ�.

2) tail

tail�� �����Ǿ� pool �ȿ� ���� ���� ���� node�� �ּҸ�,
�Ҵ�Ǿ� pool �ۿ� ���� ���� head�� ������ ���� ������.

3) ���� ���

Free�� ȣ��Ǹ� �Ʒ� ���׵��� üũ�Ѵ�.
- head�� tail�� ���� ���� ���� ��������
- pool ID�� instance ID�� �������� ���� �� �ִ� ������

�������� ���� ���, Pool�� ������ �ʴ� �������� ���� ��û,
Overflow/Underflow �� �Ѱ��� ���� �Ǵ��Ͽ� ���ܸ� ������.

2. UseCount, Capacity

UseCount�� Capacity�� ����Ͽ� �ʿ��� �� ����� �� �ֵ��� �Ѵ�.
��, Pool �Ҹ� �� UseCount�� Ȯ���Ͽ� �̹�ȯ �����Ͱ� ������ �˸���.
����� �޽����� �ֿܼ� ����ϰ� ������ ���� ������ �����̴�.

==================================================*/

template <class DATA>
class CLockFreePool
{
private:
#ifdef __OBJECT_POOL_DEBUG__
	struct stNODE
	{
		size_t head;
		DATA data;
		size_t tail;
	};
#else
	struct stNODE
	{
		DATA data;
		long threadIdx;
		size_t tail = nullptr;
	};
#endif

public:
	template<typename... Types>
	CLockFreePool(int blockNum, bool placementNew, Types... args);
	virtual	~CLockFreePool();

public:
	template<typename... Types>
	inline DATA* Alloc(Types... args);
	inline bool Free(DATA* pData);

private:
	bool _placementNew;
	int _blockNum;
	
private: // For Multithread
#define dfTHREAD_MAX 20
	DWORD _tlsIdx = 0;
	volatile long _threadIdx = 0;
	stNODE* _pFreeNode[dfTHREAD_MAX] = { nullptr, };
	HANDLE _heapHandle[dfTHREAD_MAX] = { nullptr, };
	
#ifdef __OBJECT_POOL_DEBUG__
public:
	int		GetCapacityCount(void) { return _capacity; }
	int		GetUseCount(void) { return _useCount; }

private:
	int _capacity;
	int _useCount;
	unsigned char _poolID;
#endif

};

template<class DATA>
template<typename... Types>
CLockFreePool<DATA>::CLockFreePool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum)
{
#ifdef __OBJECT_POOL_DEBUG__

	_capacity = _blockNum;
	_useCount = 0;
	_poolID = gObjectPoolID;
	gObjectPoolID++;
#endif

	_tlsIdx = TlsAlloc();

	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, threadIdx);
		_heapHandle[threadIdx] = HeapCreate(HEAP_NO_SERIALIZE, 4096, 1);
	}

	if (_blockNum <= 0)
		return;

	if (_placementNew)
	{
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�

		_pFreeNode[threadIdx] = (stNODE*)HeapAlloc(
			_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
		_pFreeNode[threadIdx]->threadIdx = threadIdx;
		_pFreeNode[threadIdx]->tail = (size_t)nullptr;

		for (int i = 1; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)HeapAlloc(
				_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));			
			p->threadIdx = threadIdx;
			p->tail = (size_t)_pFreeNode[threadIdx];

			_pFreeNode[threadIdx] = p;
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�

		_pFreeNode[threadIdx] = (stNODE*)HeapAlloc(
			_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
		_pFreeNode[threadIdx]->threadIdx = threadIdx;
		_pFreeNode[threadIdx]->tail = (size_t)nullptr;

		for (int i = 1; i < _blockNum; i++)
		{
			new (&(_pFreeNode[threadIdx]->data)) DATA(args...);

			stNODE* p = (stNODE*)HeapAlloc(
				_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			p->threadIdx = threadIdx;
			p->tail = (size_t)_pFreeNode[threadIdx];

			_pFreeNode[threadIdx] = p;
		}
		new (&(_pFreeNode[threadIdx]->data)) DATA(args...);
	}
}

template<class DATA>
CLockFreePool<DATA>::~CLockFreePool()
{
#ifdef __OBJECT_POOL_DEBUG__

	if (_useCount != 0)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: There is Unfree Data\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"ERROR", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: There is Unfree Data\n",
			_T(__FUNCTION__), __LINE__);

		dump.Crash();
	}

#endif

	if (_pFreeNode == nullptr)
		return;

	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵȴ�
		for(int i = 0; i < dfTHREAD_MAX; i++)
		{
			HeapDestroy(_heapHandle[i]);
		}
	}
	else
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		for (int i = 0; i < dfTHREAD_MAX; i++)
		{			
			while (_pFreeNode[i]->tail != (size_t)nullptr)
			{
				size_t next = _pFreeNode[i]->tail;
				(_pFreeNode[i]->data).~DATA();
				_pFreeNode[i] = (stNODE*)next;
			}
			(_pFreeNode[i]->data).~DATA();

			HeapDestroy(_heapHandle[i]);
		}
	}

	TlsFree(_tlsIdx);
}

template<class DATA>
template<typename... Types>
DATA* CLockFreePool<DATA>::Alloc(Types... args)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, threadIdx);
		_heapHandle[threadIdx] = HeapCreate(HEAP_NO_SERIALIZE, 4096, 1);
	}

	if (_pFreeNode == nullptr)
	{
		// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ� (���� ����)

		stNODE* pNew = (stNODE*)HeapAlloc(
			_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));

		new (&(pNew->data)) DATA(args...);
		pNew->threadIdx = threadIdx;
		pNew->tail = (size_t)_pFreeNode[threadIdx];

#ifdef __OBJECT_POOL_DEBUG__

		_capacity++;
		_useCount++;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)(&(pNew->data));

		pNew->head = code;
		pNew->tail = code;

#endif

		return &(pNew->data);
	}

	if (_placementNew)
	{
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�

		stNODE* p = _pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)_pFreeNode[threadIdx]->tail;
		new (&(p->data)) DATA(args...);

#ifdef __OBJECT_POOL_DEBUG__

		_useCount++;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)(&(p->data));

		p->head = code;
		p->tail = code;
#endif

		return &(p->data);
	}
	else
	{
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ������ �ʴ´� 
		stNODE* p = _pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)_pFreeNode->tail;

#ifdef __OBJECT_POOL_DEBUG__

		_useCount++;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)(&(p->data));

		p->head = code;
		p->tail = code;

#endif

		return &(p->data);
	}

	return nullptr;
}

template<class DATA>
bool CLockFreePool<DATA>::Free(DATA* pData)
{
	int threadIdx = ((stNODE*)pData)->threadIdx;

	if (_placementNew)
	{
		// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNode�� push�Ѵ�

#ifdef __OBJECT_POOL_DEBUG__

		_useCount--;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)pData;


		size_t offset = (size_t)(&(((stNODE*)nullptr)->data));
		stNODE* pNode = (stNODE*)((size_t)pData - offset);

		if (pNode->head != code || pNode->tail != code)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Code is Diffrent. code %o, head %o, tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->tail);

			::wprintf(L"%s[%d]: Code is Diffrent. code %o, head %o, tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->tail);

			dump.Crash();
		}

		pData->~DATA();
		pNode->tail = (size_t)_pFreeNode;
		_pFreeNode = pNode;

#else

		pData->~DATA();
		((stNODE*)pData)->tail = (size_t)_pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)pData;

#endif
		return true;
	}
	else
	{
		// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNode�� push�Ѵ�

#ifdef __OBJECT_POOL_DEBUG__

		_useCount--;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)pData;

		size_t offset = (size_t)(&(((stNODE*)nullptr)->data));
		stNODE* pNode = (stNODE*)((size_t)pData - offset);

		if (pNode->head != code || pNode->tail != code)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Code is Different. code %o, head %o, tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->tail);

			::wprintf(L"%s[%d]: Code is Different. code %o, head %o, tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->tail);

			dump.Crash();
			return false;
		}

		pNode->tail = (size_t)_pFreeNode;
		_pFreeNode = pNode;

#else

		((stNODE*)pData)->tail = (size_t)_pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)pData;

#endif
		return true;
	}
	return false;
}

#endif

