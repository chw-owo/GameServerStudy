#pragma once
#include "CTlsPool.h"

template<typename T>
class CLockFreeStack
{
#define __ADDRESS_BIT__ 47
private:
	class StackNode
	{
		friend CLockFreeStack;
	public:
		bool operator != (const StackNode& other)
		{
			return _data != other._data;
		}

	public:
		StackNode() {}
		~StackNode() {}

	private:
		T _data = 0;
		__int64 _next = NULL;
	};

private: // For protect ABA
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

public:
	CLockFreeStack()
	{
		_pPool = new CTlsPool<StackNode>(0, false);
		_keyMask = 0b11111111111111111;
		_addressMask = 0b11111111111111111;
		_addressMask <<= __ADDRESS_BIT__;
		_addressMask = ~_addressMask;
	}

private:
	__int64 _pTop = NULL;
	CTlsPool<StackNode>* _pPool = nullptr;

private:
	volatile long _useSize = 0;
public:
	long GetUseSize()
	{
		return _useSize;
	}

public:
	void Push(T data)
	{
		StackNode* pNewNode = _pPool->Alloc();
		pNewNode->_data = data;

		// For Protect ABA
		unsigned __int64 ret = (unsigned __int64)InterlockedIncrement64(&_key);
		unsigned __int64 key = ret;
		key <<= __ADDRESS_BIT__;
		__int64 pNewTop = (__int64)pNewNode;
		pNewTop &= _addressMask;
		unsigned __int64 tmp = pNewTop;
		pNewTop |= key;

		for (;;)
		{
			__int64 pPrevTop = _pTop;
			pNewNode->_next = pPrevTop;

			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				InterlockedIncrement(&_useSize);
				return;
			}
		}
	}

	T Pop()
	{
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			__int64 pNextTop = ((StackNode*)(pPrevTop & _addressMask))->_next;

			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				T data = ((StackNode*)(pPrevTop & _addressMask))->_data;
				_pPool->Free((StackNode*)(pPrevTop & _addressMask));
				InterlockedDecrement(&_useSize);
				return data;
			}
		}
	}
};

