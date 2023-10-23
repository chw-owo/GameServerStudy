#pragma once
#include "CLockFreePool.h"

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
#define __USESIZE_64BIT__ 47
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

public:
	CLockFreeStack()
	{
		_pPool = new CLockFreePool<StackNode>(0, false);
		_keyMask = 0b11111111111111111;
		_addressMask = 0b11111111111111111;
		_addressMask <<= __USESIZE_64BIT__;
		_addressMask = ~_addressMask;
	}

private:
	__int64 _pTop = NULL;
	CLockFreePool<StackNode>* _pPool = nullptr;

public:
	void Push(T data)
	{
		StackNode* pNewNode = _pPool->Alloc();
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
				LeaveLog(0, (unsigned __int64)pNewTop, (unsigned __int64)pPrevTop);
				return;
			}
		}
	}

	T Pop()
	{
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == 0) return 0;

			__int64 pNextTop = ((StackNode*)(pPrevTop & _addressMask))->_next;
			T data = pPrevTop->_data;

			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(1, (unsigned __int64)pNextTop, (unsigned __int64)pPrevTop);
				_pPool->Free((StackNode*)(pPrevTop & _addressMask));
				return data;
			}
		}
	}

private:
	class StackDebugData
	{
		friend CLockFreeStack;

	private:
		StackDebugData() : _idx(-1), _datahreadID(-1), _line(-1),
			_compKey(-1), _exchKey(-1), _compAddress(-1), _exchAddress(-1) {}

		void SetData(int idx, int threadID, int line,
			int exchKey, int compKey, __int64 exchAddress, __int64 compAddress)
		{
			_exchKey = exchKey;
			_exchAddress = exchAddress;

			_compKey = compKey;
			_compAddress = compAddress;

			_idx = idx;
			_datahreadID = threadID;
			_line = line;
		}

	private:
		int _idx;
		int _datahreadID;
		int _line;
		int _compKey;
		int _exchKey;
		__int64 _compAddress;
		__int64 _exchAddress;
	};

private:
#define dfSTACK_DEBUG_MAX 1000
	StackDebugData _stackDebugArray[dfSTACK_DEBUG_MAX];
	volatile long _stackDebugIdx = -1;

private:
	inline void LeaveLog(int line, unsigned __int64 exchange, unsigned __int64 comperand)
	{
		LONG idx = InterlockedIncrement(&_stackDebugIdx);

		int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
		int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
		__int64 exchAddress = exchange & _addressMask;
		__int64 compAddress = comperand & _addressMask;

		_stackDebugArray[idx % dfSTACK_DEBUG_MAX].SetData(
			idx, GetCurrentThreadId(), line, exchKey, compKey, exchAddress, compAddress);
	}
};

