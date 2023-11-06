#pragma once
#include "CTlsPool.h"

template<typename T>
class CLockFreeStack
{
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
		_addressMask <<= __USESIZE_64BIT__;
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
		StackNode* pNewNode = _pPool->Alloc(5);
		pNewNode->_data = data;

		// For Protect ABA
		unsigned __int64 ret = (unsigned __int64)InterlockedIncrement64(&_key);
		unsigned __int64 key = ret;
		key <<= __USESIZE_64BIT__;
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
				_pPool->Free(6, (StackNode*)(pPrevTop & _addressMask));
				InterlockedDecrement(&_useSize);
				return data;
			}
		}
	}


private:
	class StackDebugData
	{
		friend CLockFreeStack;

	private:
		StackDebugData() : _idx(-1), _threadID(-1), _line(-1),
			_compKey(-1), _exchKey(-1), _compAddress(-1), _exchAddress(-1) {}

		void SetData(int idx, int threadID, int line, int exchKey, int compKey,
			__int64 exchAddress, __int64 compAddress, T exchVal, T compVal)
		{
			_exchVal = exchVal;
			_exchKey = exchKey;
			_exchAddress = exchAddress;

			_compVal = compVal;
			_compKey = compKey;
			_compAddress = compAddress;

			_idx = idx;
			_threadID = threadID;
			_line = line;
		}

	private:
		int _idx;
		int _threadID;
		int _line;
		int _compKey;
		int _exchKey;
		__int64 _compAddress;
		__int64 _exchAddress;
		T _compVal;
		T _exchVal;
	};

};

