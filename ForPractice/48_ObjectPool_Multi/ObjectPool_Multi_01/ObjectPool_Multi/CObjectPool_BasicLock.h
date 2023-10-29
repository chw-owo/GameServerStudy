#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>

#include "CProfiler.h"

template <class DATA>
class CObjectPool_BasicLock
{
private:
	struct stNODE
	{
		DATA data;
		size_t tail = NULL;
	};

public:
	template<typename... Types>
	CObjectPool_BasicLock(int blockNum, bool placementNew, Types... args);
	virtual	~CObjectPool_BasicLock();

public:
	template<typename... Types>
	inline DATA* Alloc(Types... args);
	inline bool Free(DATA* pData);

private:
	bool _placementNew;
	int _blockNum;
	stNODE* _pFreeNode = nullptr;

private:
	SRWLOCK _lock;
};

template<class DATA>
template<typename... Types>
CObjectPool_BasicLock<DATA>::CObjectPool_BasicLock(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pFreeNode(nullptr)
{
	InitializeSRWLock(&_lock);

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
CObjectPool_BasicLock<DATA>::~CObjectPool_BasicLock()
{
	if (_pFreeNode == nullptr)
		return;

	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵȴ�
		while (_pFreeNode->tail != NULL)
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
		while (_pFreeNode->tail != NULL)
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
DATA* CObjectPool_BasicLock<DATA>::Alloc(Types... args)
{
	PRO_BEGIN(L"Alloc: Acquire Lock");
	AcquireSRWLockExclusive(&_lock);
	PRO_END(L"Alloc: Acquire Lock");

	if (_pFreeNode == nullptr)
	{
		// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ� (���� ����)
		PRO_BEGIN(L"Alloc: Empty");
		stNODE* pNew = (stNODE*)malloc(sizeof(stNODE));
		new (&(pNew->data)) DATA(args...);
		PRO_END(L"Alloc: Empty");

		ReleaseSRWLockExclusive(&_lock);
		return &(pNew->data);
	}

	if (_placementNew)
	{
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�
		PRO_BEGIN(L"Alloc: Alloc");
		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;
		new (&(p->data)) DATA(args...);
		PRO_END(L"Alloc: Alloc");

		ReleaseSRWLockExclusive(&_lock);
		return &(p->data);
	}
	else
	{
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ������ �ʴ´�
		PRO_BEGIN(L"Alloc: Alloc");
		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;
		PRO_END(L"Alloc: Alloc");

		ReleaseSRWLockExclusive(&_lock);
		return &(p->data);
	}

	ReleaseSRWLockExclusive(&_lock);
	return nullptr;
}

template<class DATA>
bool CObjectPool_BasicLock<DATA>::Free(DATA* pData)
{
	PRO_BEGIN(L"Free: Acquire Lock");
	AcquireSRWLockExclusive(&_lock);
	PRO_END(L"Free: Acquire Lock");

	if (_placementNew)
	{
		// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNode�� push�Ѵ�
		PRO_BEGIN(L"Free: Free");
		pData->~DATA();
		((stNODE*)pData)->tail = (size_t)_pFreeNode;
		_pFreeNode = (stNODE*)pData;
		PRO_END(L"Free: Free");

		ReleaseSRWLockExclusive(&_lock);
		return true;
	}
	else
	{
		// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNode�� push�Ѵ�
		PRO_BEGIN(L"Free: Free");
		((stNODE*)pData)->tail = (size_t)_pFreeNode;
		_pFreeNode = (stNODE*)pData;
		PRO_END(L"Free: Free");

		ReleaseSRWLockExclusive(&_lock);
		return true;
	}

	ReleaseSRWLockExclusive(&_lock);
	return false;
}
