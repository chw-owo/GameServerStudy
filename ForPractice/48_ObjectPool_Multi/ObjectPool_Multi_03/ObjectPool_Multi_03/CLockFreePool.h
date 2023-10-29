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
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다
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
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다
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
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
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
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다
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
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == NULL)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
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
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출하지 않는다
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == NULL)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
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
		// Data의 소멸자를 호출한 후 _pTop에 push한다
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
		// Data의 소멸자를 호출하지 않고 _pTop에 push한다
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
