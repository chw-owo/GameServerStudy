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
		StackNode(): _data() {}
		~StackNode() {}

	private:
		T _data = 0;
		StackNode* _next = nullptr;
	};

public:
	static CTlsPool<StackNode>* _pNodePool;

private: // For protect ABA
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

private:
	// 17 bit key + 47 bit address
	inline __int64 GetUniqueAddress(StackNode* node)
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
		StackNode* pNewNode = _pNodePool->Alloc();
		pNewNode->_data = data;
		__int64 newTop = GetUniqueAddress(pNewNode);

		for (;;)
		{
			__int64 pPrevTop = _pTop;
			pNewNode->_next = ((StackNode*)(pPrevTop & _addressMask));

			if (InterlockedCompareExchange64(&_pTop, newTop, pPrevTop) == pPrevTop)
			{
				return;
			}
		}
	}

	inline T Pop()
	{
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			StackNode* pPrevNode = (StackNode*)(pPrevTop & _addressMask);
			if (pPrevNode == nullptr) return 0;
			__int64 pNewTop = GetUniqueAddress(pPrevNode->_next);

			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				T data = pPrevNode->_data;
				_pNodePool->Free(pPrevNode);
				return data;
			}
		}
	}
};

#define dfTHREAD_CNT 10
#define dfTEST_TOTAL_CNT 2500000

template<typename T>
CTlsPool<typename CLockFreeStack<T>::StackNode>* CLockFreeStack<T>::_pNodePool 
	= new CTlsPool<CLockFreeStack<T>::StackNode>(dfTHREAD_CNT * dfTEST_TOTAL_CNT, true);