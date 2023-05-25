#pragma once
#ifndef  __CHUU_MEMORY_POOL_TYPE__
#define  __CHUU_MEMORY_POOL_TYPE__
#include <new.h>
#include <stdlib.h>

/* ===============================================

<< MemoryPool in the Project >>

�� ������Ʈ������ ����

List.h�� Node,
Network.h�� Session,
PlayerManager.h�� Player

�� �������� �޸� Ǯ�� ���� �����ϰ� �ִ�.

����ȭ ��Ŷ�� ���������� ����ϹǷ� ������ �������� �ʴ´�. 

==================================================*/

#define __CHUU_MEMORY_POOL_TYPE_DEBUG__

/* ===============================================

<< __CHUU_MEMORY_POOL_TYPE_DEBUG__ >>

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

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
#include <stdio.h>
extern unsigned char gObjectPoolID; 
// TO-DO: ���� ����.. �����ϱ�
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

bPlacementNew�� True�� ���,
Alloc, Free�� �� ������ ������, �Ҹ��ڸ� ȣ���Ѵ�.
�̶� Alloc�� ���ڷ� �������� ���ڸ� ������ �� �ִ�.

bPlacementNew�� False�� ���,
���������� malloc, free�� �� �� ������, �Ҹ��ڸ� ȣ���Ѵ�.
�̶� ������ ���ڸ� ������ �� ������, �⺻ �����ڰ� ȣ��ȴ�.

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
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�

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
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�

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
DATA* CMemoryPoolT<DATA>::Alloc(Types... data)
{
	if (_pFreeNode == nullptr)
	{
		// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ� (���� ����)

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
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�

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
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ������ �ʴ´�

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
		// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNode�� push�Ѵ�

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
		// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNode�� push�Ѵ�

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