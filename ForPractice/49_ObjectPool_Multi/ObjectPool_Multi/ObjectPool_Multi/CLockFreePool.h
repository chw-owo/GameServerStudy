#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>

template <class DATA>
class CLockFreePool
{
private:
	struct PoolNode
	{
		DATA _data;
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
	inline DATA* Alloc(Types... args);
	inline bool Free(DATA* pData);

private:
	bool _placementNew;
	int _blockNum;
	__int64 _pTop = 0;
};

template<class DATA>
template<typename... Types>
CLockFreePool<DATA>::CLockFreePool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pTop(NULL)
{

	if (_blockNum <= 0)
		return;

	_keyMask = 0b11111111111111111;
	_addressMask = 0b11111111111111111;
	_addressMask <<= __USESIZE_64BIT__;
	_addressMask = ~_addressMask;

	if (_placementNew)
	{
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다

		PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));

		// For Protect ABA
		unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
		key <<= __USESIZE_64BIT__;
		__int64 newTop = (__int64)pNewNode;
		newTop &= _addressMask;
		newTop |= key;

		_pTop = newTop;

		for (int i = 1; i < _blockNum; i++)
		{
			pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
			pNewNode->_next = _pTop;

			key = (unsigned __int64)InterlockedIncrement64(&_key);
			key <<= __USESIZE_64BIT__;
			__int64 newTop = (__int64)pNewNode;
			newTop &= _addressMask;
			newTop |= key;

			_pTop = newTop;
		}
	}
	else
	{
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다

		PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
		_pTop = (__int64)pNewNode;

		for (int i = 1; i < _blockNum; i++)
		{
			new (&(((PoolNode*)_pTop)->_data)) DATA(args...);
			PoolNode* p = (PoolNode*)malloc(sizeof(PoolNode));
			p->_next = _pTop;
			_pTop = (__int64)p;
		}
		new (&(((PoolNode*)_pTop)->_data)) DATA(args...);
	}
}

// TO-DO: 소멸자 Error
template<class DATA>
CLockFreePool<DATA>::~CLockFreePool()
{
	if (_pTop == NULL)
		return;

	if (_placementNew)
	{
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
		__int64 next = ((PoolNode*)(_pTop & _addressMask))->_next;
		while ((next & _addressMask) != 0)
		{
			free(((PoolNode*)(_pTop & _addressMask)));
			_pTop = next;
			next = ((PoolNode*)(_pTop & _addressMask))->_next;
		}
		free(((PoolNode*)(_pTop & _addressMask)));
	}
	else
	{
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다
		int idx = 0;
		__int64 next = ((PoolNode*)(_pTop & _addressMask))->_next;
		while ((next & _addressMask) != 0)
		{
			idx++;
			(((PoolNode*)(_pTop & _addressMask))->_data).~DATA();
			free(((PoolNode*)(_pTop & _addressMask))); 
			_pTop = next;
			next = ((PoolNode*)(_pTop & _addressMask))->_next;
		}
		(((PoolNode*)(_pTop & _addressMask))->_data).~DATA();
		free(((PoolNode*)(_pTop & _addressMask)));
	}
}

template<class DATA>
template<typename... Types>
DATA* CLockFreePool<DATA>::Alloc(Types... args) // Pop
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
				new (&(pNewNode->_data)) DATA(args...);
				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next;
			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				new (&(((PoolNode*)(pPrevTop & _addressMask))->_data)) DATA(args...);
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

template<class DATA>
bool CLockFreePool<DATA>::Free(DATA* pData) // Push
{
	// For Protect ABA
	unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
	key <<= __USESIZE_64BIT__;
	__int64 pNewTop = (__int64)pData;
	pNewTop &= _addressMask;
	unsigned __int64 tmp = pNewTop;
	pNewTop |= key;

	if (_placementNew)
	{
		// Data의 소멸자를 호출한 후 _pTop에 push한다
		pData->~DATA();
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
