#pragma once
#ifndef  __OBJECT_POOL__
#define  __OBJECT_POOL__
#include <new.h>
#include <stdlib.h>
#include <wchar.h>
#include "SystemLog.h"

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

#ifdef __OBJECT_POOL_DEBUG__
#include <stdio.h>
extern unsigned char gObjectPoolID;
#endif


template <class DATA>
class CObjectPool
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
		size_t tail = nullptr;
	};
#endif

public:
	template<typename... Types>
	CObjectPool(int blockNum, bool placementNew, Types... args);
	virtual	~CObjectPool();

public:
	template<typename... Types>
	inline DATA* Alloc(Types... args);
	inline bool Free(DATA* pData);

private:
	bool _placementNew;
	int _blockNum;
	stNODE* _pFreeNode = nullptr;

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
CObjectPool<DATA>::CObjectPool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pFreeNode(nullptr)
{
#ifdef __OBJECT_POOL_DEBUG__

	_capacity = _blockNum;
	_useCount = 0;
	_poolID = gObjectPoolID;
	gObjectPoolID++;
#endif

	if (_blockNum <= 0)
		return;

	if (_placementNew)
	{
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�

		_pFreeNode = (stNODE*)malloc(sizeof(stNODE));
		_pFreeNode->tail = (size_t)nullptr;
		for (int i = 1; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)malloc(sizeof(stNODE));
			p->tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�

		_pFreeNode = (stNODE*)malloc(sizeof(stNODE));
		_pFreeNode->tail = (size_t)nullptr;
		for (int i = 1; i < _blockNum; i++)
		{
			new (&(_pFreeNode->data)) DATA(args...);
			stNODE* p = (stNODE*)malloc(sizeof(stNODE));
			p->tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
		new (&(_pFreeNode->data)) DATA(args...);
	}
}

template<class DATA>
CObjectPool<DATA>::~CObjectPool()
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

		
	}

#endif

	if (_pFreeNode == nullptr)
		return;

	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵȴ�

		while (_pFreeNode->tail != (size_t)nullptr)
		{
			size_t next = _pFreeNode->tail;
			free(_pFreeNode);
			_pFreeNode = (stNODE*)next;
		}
		free(_pFreeNode);
	}
	else
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�

		while (_pFreeNode->tail != (size_t)nullptr)
		{
			size_t next = _pFreeNode->tail;
			(_pFreeNode->data).~DATA();
			free(_pFreeNode);
			_pFreeNode = (stNODE*)next;
		}
		(_pFreeNode->data).~DATA();
		free(_pFreeNode);
	}
}

template<class DATA>
template<typename... Types>
DATA* CObjectPool<DATA>::Alloc(Types... args)
{
	if (_pFreeNode == nullptr)
	{
		// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ� (���� ����)

		stNODE* pNew = (stNODE*)malloc(sizeof(stNODE));
		new (&(pNew->data)) DATA(args...);

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

		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;
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

		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;

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
bool CObjectPool<DATA>::Free(DATA* pData)
{
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

			
		}

		pData->~DATA();
		pNode->tail = (size_t)_pFreeNode;
		_pFreeNode = pNode;

#else

		pData->~DATA();
		((stNODE*)pData)->tail = (size_t)_pFreeNode;
		_pFreeNode = (stNODE*)pData;

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

			
			return false;
		}

		pNode->tail = (size_t)_pFreeNode;
		_pFreeNode = pNode;

#else

		((stNODE*)pData)->tail = (size_t)_pFreeNode;
		_pFreeNode = (stNODE*)pData;

#endif
		return true;
	}
	return false;
}

#endif

