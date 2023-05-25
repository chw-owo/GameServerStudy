#pragma once
#ifndef  __CHUU_MEMORY_POOL_TYPE__
#define  __CHUU_MEMORY_POOL_TYPE__
#include <new.h>
#include <stdlib.h>

/* ===============================================

<< MemoryPool in the Project >>

이 프로젝트에서는 현재

List.h의 Node,
Network.h의 Session,
PlayerManager.h의 Player

이 세가지를 메모리 풀을 통해 관리하고 있다.

직렬화 패킷은 지역에서만 사용하므로 별도로 관리하지 않는다. 

==================================================*/

#define __CHUU_MEMORY_POOL_TYPE_DEBUG__

/* ===============================================

<< __CHUU_MEMORY_POOL_TYPE_DEBUG__ >>

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

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
#include <stdio.h>
extern unsigned char gObjectPoolID; 
// TO-DO: 전역 말고.. 수정하기
#endif

template <class DATA>
class CMemoryPoolT
{
private:

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

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
	CMemoryPoolT();
	CMemoryPoolT(int iBlockNum, bool bPlacementNew = false);
	virtual	~CMemoryPoolT();

public:
	template<typename... Types>
	DATA* Alloc(Types... data);
	bool  Free(DATA* pData);

private:
	bool _bPlacementNew;
	int _iBlockNum;
	stNODE* _pFreeNode = nullptr;

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
public:
	int		GetCapacityCount(void) { return _iCapacity; }
	int		GetUseCount(void) { return _iUseCount; }

private:
	int _iCapacity;
	int _iUseCount;
	unsigned char _iPoolID;
#endif

};

/* ===============================================

<< bPlacementNew >>

bPlacementNew가 True일 경우,
Alloc, Free가 될 때마다 생성자, 소멸자를 호출한다.
이때 Alloc의 인자로 생성자의 인자를 전달할 수 있다.

bPlacementNew가 False일 경우,
내부적으로 malloc, free가 될 때 생성자, 소멸자를 호출한다.
이때 생성자 인자를 전달할 수 없으며, 기본 생성자가 호출된다.

==================================================*/

template<class DATA>
CMemoryPoolT<DATA>::CMemoryPoolT()
	:_bPlacementNew(true), _iBlockNum(0), _pFreeNode(nullptr) 
{
#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

	_iCapacity = _iBlockNum;
	_iUseCount = 0;
	_iPoolID = gObjectPoolID;
	gObjectPoolID++;

#endif
}

template<class DATA>
CMemoryPoolT<DATA>::CMemoryPoolT(int iBlockNum, bool bPlacementNew)
	:_bPlacementNew(bPlacementNew), _iBlockNum(iBlockNum), _pFreeNode(nullptr)
{
#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

	_iCapacity = _iBlockNum;
	_iUseCount = 0;
	_iPoolID = gObjectPoolID;
	gObjectPoolID++;

#endif

	if (_iBlockNum <= 0)
		return;

	if (_bPlacementNew)
	{
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다

		_pFreeNode = (stNODE*)malloc(sizeof(stNODE));
		_pFreeNode->tail = (size_t)nullptr;
		for (int i = 1; i < _iBlockNum; i++)
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
		for (int i = 1; i < _iBlockNum; i++)
		{
			new (&(_pFreeNode->data)) DATA();
			stNODE* p = (stNODE*)malloc(sizeof(stNODE));
			p->tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
		new (&(_pFreeNode->data)) DATA();
	}
}

template<class DATA>
CMemoryPoolT<DATA>::~CMemoryPoolT()
{
#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

	if (_iUseCount != 0)
	{
		printf("There is Unfree Data!!\n");
	}

#endif

	if (_pFreeNode == nullptr)
		return;

	if (_bPlacementNew)
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
DATA* CMemoryPoolT<DATA>::Alloc(Types... data)
{
	if (_pFreeNode == nullptr)
	{
		// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)

		stNODE* pNew = (stNODE*)malloc(sizeof(stNODE));
		new (&(pNew->data)) DATA(data...);

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

		_iCapacity++;
		_iUseCount++;

		size_t code = 0;
		code |= (size_t)_iPoolID << (3 * 3);
		code |= 0777 & (size_t)(&(pNew->data));

		pNew->head = code;
		pNew->tail = code;

#endif

		return &(pNew->data);
	}

	if (_bPlacementNew)
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다

		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;
		new (&(p->data)) DATA(data...);

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

		_iUseCount++;

		size_t code = 0;
		code |= (size_t)_iPoolID << (3 * 3);
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

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

		_iUseCount++;

		size_t code = 0;
		code |= (size_t)_iPoolID << (3 * 3);
		code |= 0777 & (size_t)(&(p->data));

		p->head = code;
		p->tail = code;

#endif

		return &(p->data);
	}

	return nullptr;
}

template<class DATA>
bool CMemoryPoolT<DATA>::Free(DATA* pData)
{
	if (_bPlacementNew)
	{
		// Data의 소멸자를 호출한 후 _pFreeNode에 push한다

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

		_iUseCount--;

		size_t code = 0;
		code |= (size_t)_iPoolID << (3 * 3);
		code |= 0777 & (size_t)pData;


		size_t offset = (size_t)(&(((stNODE*)nullptr)->data));
		stNODE* pNode = (stNODE*)((size_t)pData - offset);

		if (pNode->head != code || pNode->tail != code)
		{
			printf("Error!code % o, head % o, tail % o\n",
				code, pNode->head, pNode->tail);
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

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

		_iUseCount--;

		size_t code = 0;
		code |= (size_t)_iPoolID << (3 * 3);
		code |= 0777 & (size_t)pData;

		size_t offset = (size_t)(&(((stNODE*)nullptr)->data));
		stNODE* pNode = (stNODE*)((size_t)pData - offset);

		if (pNode->head != code || pNode->tail != code)
		{
			printf("Error! code %o, head %o, tail %o\n",
				code, pNode->head, pNode->tail);

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