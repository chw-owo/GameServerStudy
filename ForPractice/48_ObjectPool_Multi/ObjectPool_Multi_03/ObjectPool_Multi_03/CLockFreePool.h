#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>

template <class T>
class CLockFreePool
{
private:
	struct PoolNode
	{
		T _data;
		__int64 _next = NULL;
	};

private: // For protect ABA
#define __USESIZE_64BIT__ 47
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

public:
	template<typename... Types>
	CLockFreePool(int blockNum, bool placementNew, Types... args);
	virtual	~CLockFreePool();

public:
	template<typename... Types>
	inline T* Alloc(Types... args);
	inline bool Free(T* pData);

private:
	bool _placementNew;
	int _blockNum;
	_int64 _pTop = NULL;
};

template<class T>
template<typename... Types>
CLockFreePool<T>::CLockFreePool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pTop(NULL)
{
	_keyMask = 0b11111111111111111;
	_addressMask = 0b11111111111111111;
	_addressMask <<= __USESIZE_64BIT__;
	_addressMask = ~_addressMask;

	if (_blockNum <= 0)
		return;

	if (_placementNew)
	{
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�
		for (int i = 0; i < _blockNum; i++)
		{
			PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
			pNewNode->_next = _pTop;

			unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
			key <<= __USESIZE_64BIT__;
			unsigned __int64 newTop = (__int64)pNewNode;
			newTop &= _addressMask;
			newTop |= key;

			_pTop = newTop;
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		for (int i = 0; i < _blockNum; i++)
		{
			PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
			new (&(pNewNode->_data)) T(args...);
			pNewNode->_next = _pTop;

			unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
			key <<= __USESIZE_64BIT__;
			unsigned __int64 newTop = (__int64)pNewNode;
			newTop &= _addressMask;
			newTop |= key;

			_pTop = newTop;
		}
	}
}

template<class T>
CLockFreePool<T>::~CLockFreePool()
{
	if (_pTop == NULL)
		return;

	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵȴ�
		PoolNode* pTop = (PoolNode*)(_pTop & _addressMask);
		__int64 next = pTop->_next;

		while (next != NULL)
		{
			free(pTop);
			pTop = (PoolNode*)(next & _addressMask);
			next = pTop->_next;
		}
		free(pTop);
	}
	else
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		PoolNode* pTop = (PoolNode*)(_pTop & _addressMask);
		__int64 next = pTop->_next;

		while (next != NULL)
		{
			(pTop->_data).~T();
			free(pTop);
			pTop = (PoolNode*)(next & _addressMask);
			next = pTop->_next;
		}

		(pTop->_data).~T();
		free(pTop);
	}
}

template<class T>
template<typename... Types>
T* CLockFreePool<T>::Alloc(Types... args) // Pop
{
	if (_placementNew)
	{
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == NULL)
			{
				// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ� (���� ����)
				PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
				new (&(pNewNode->_data)) T(args...);
				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next;
			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				new (&(((PoolNode*)(pPrevTop & _addressMask))->_data)) T(args...);
				return &(((PoolNode*)(pPrevTop & _addressMask))->_data);
			}
		}
	}
	else
	{
		// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ������ �ʴ´�
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == NULL)
			{
				// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ� (���� ����)
				PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
				new (&(pNewNode->_data)) T(args...);
				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next;
			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				return &(((PoolNode*)(pPrevTop & _addressMask))->_data);
			}
		}
	}
	return nullptr;
}

template<class T>
bool CLockFreePool<T>::Free(T* pData) // Push
{
	// For Protect ABA
	unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
	key <<= __USESIZE_64BIT__;
	__int64 pNewTop = (__int64)pData;
	pNewTop &= _addressMask;
	pNewTop |= key;

	if (_placementNew)
	{
		// Data�� �Ҹ��ڸ� ȣ���� �� _pTop�� push�Ѵ�
		pData->~T();
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			((PoolNode*)pData)->_next = pPrevTop;
			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
				return true;
		}
	}
	else
	{
		// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pTop�� push�Ѵ�
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			((PoolNode*)pData)->_next = pPrevTop;
			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
				return true;
		}
	}
	return false;
}
