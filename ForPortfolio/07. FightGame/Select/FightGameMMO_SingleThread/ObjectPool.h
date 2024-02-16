#pragma once
#ifndef  __OBJECT_POOL__
#define  __OBJECT_POOL__
#include <new.h>
#include <stdlib.h>
#include <wchar.h>
#include "SystemLog.h"

//#define __OBJECT_POOL_DEBUG__

/* ===============================================

<< __OBJECT_POOL_DEBUG__ 설명 >>

1. Node

head와 tail에 검증 및 디버깅을 위한 값이 들어간다.
head, tail은 할당된 데이터 관리를 위한 것이므로
Alloc에서 값이 정해지고 Free에서 확인한다.

이 모드가 아닐 때는 tail에 다음 node의 주소값만 들어간다.

1) head

32bit일 경우, 상위 1byte로 Object pool ID를
하위 3byte로 Data 주소의 하위 3byte를 가진다.
64bit일 경우, 상위 4byte로 0x0000을,
하위 4byte로 32bit에서의 head 값을 가진다.

Object pool ID는 unsigned char(0~255)이며
그 이상의 Object pool이 만들어지는 것은 대비하지 않았다.

2) tail

tail는 해제되어 pool 안에 있을 때는 다음 node의 주소를,
할당되어 pool 밖에 있을 때는 head와 동일한 값을 가진다.

3) 검증 방법

Free가 호출되면 아래 사항들을 체크한다.
- head와 tail이 서로 같은 값을 가지는지
- pool ID와 instance ID를 바탕으로 나올 수 있는 값인지

만족하지 않을 경우, Pool에 속하지 않는 데이터의 해제 요청,
Overflow/Underflow 중 한가지 경우로 판단하여 예외를 던진다.

2. UseCount, Capacity

UseCount와 Capacity를 계산하여 필요할 때 출력할 수 있도록 한다.
또, Pool 소멸 시 UseCount를 확인하여 미반환 데이터가 있으면 알린다.
현재는 메시지를 콘솔에 출력하고 있으며 추후 변경할 예정이다.

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
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다

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
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다

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
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다

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
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다

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
		// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)

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
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다

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
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출하지 않는다

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
		// Data의 소멸자를 호출한 후 _pFreeNode에 push한다

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
		// Data의 소멸자를 호출하지 않고 _pFreeNode에 push한다

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

