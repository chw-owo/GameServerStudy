#pragma once
#include "CTlsPool.h"

#define dfSTACKNODE_DEF 40000
template<typename T>
class CLockFreeStack
{
#define __ADDRESS_BIT__ 47
private:
	class StackNode
	{
		friend CLockFreeStack;
	public:
		StackNode() {}
		~StackNode() {}

	private:
		T _data = 0;
		__int64 _next = NULL;
	};

public:
	static CTlsPool<StackNode>* _pStackPool;

private: // For protect ABA
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

private:
	// 17 bit key + 47 bit address
	inline __int64 CreateAddress(StackNode* node)
	{
		unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
		key <<= __ADDRESS_BIT__;
		__int64 address = (__int64)node;
		address &= _addressMask;
		address |= key;
		return address;
	}

public:
	inline CLockFreeStack()
	{
		_pStackPool = new CTlsPool<StackNode>(0, false);
		_keyMask = 0b11111111111111111;
		_addressMask = 0b11111111111111111;
		_addressMask <<= __ADDRESS_BIT__;
		_addressMask = ~_addressMask;
	}

private:
	__int64 _pTop = NULL;
	volatile long _useSize = 0;

public:
	inline long GetUseSize() { return _useSize; }

public:
	inline void Push(T data)
	{
		StackNode* pNewNode = _pStackPool->Alloc();
		pNewNode->_data = data;
		__int64 newTop = CreateAddress(pNewNode);

		for (;;)
		{
			__int64 pPrevTop = _pTop;
			pNewNode->_next = pPrevTop;

			if (InterlockedCompareExchange64(&_pTop, newTop, pPrevTop) == pPrevTop)
			{
				InterlockedIncrement(&_useSize);
				return;
			}
		}
	}

	inline T Pop()
	{
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			__int64 pNextTop = ((StackNode*)(pPrevTop & _addressMask))->_next;

			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				InterlockedDecrement(&_useSize);
				StackNode* pPrevNode = (StackNode*)(pPrevTop & _addressMask);
				T data = pPrevNode->_data;
				_pStackPool->Free(pPrevNode);
				return data;
			}
		}
	}
};

template<typename T>
CTlsPool<typename CLockFreeStack<T>::StackNode>* CLockFreeStack<T>::_pStackPool = new CTlsPool<CLockFreeStack<T>::StackNode>(dfSTACKNODE_DEF, false);