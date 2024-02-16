#pragma once
#include <Windows.h>
#define dfDEBUG_MAX 1000

template<typename T>
class CLockFreeStack
{
private:
	class StackNode
	{
		friend CLockFreeStack;
	public:
		StackNode(T data) : _data(data) {}
		~StackNode() {}

	private:
		T _data = 0;
		StackNode* _next = nullptr;
	};

private:
	class DebugData
	{
	public:
		int _threadID = 0;
		int _function = 0;
		PVOID _pPrevTop = 0;
		PVOID _pNewTop = 0;
	};

	/*
	void LeaveLog(PVOID pPrevTop, PVOID pNewTop, int function)
	{
		long idx = InterlockedIncrement(&_idx);

		_debugs[idx % dfDEBUG_MAX]._threadID = GetCurrentThreadId();
		_debugs[idx % dfDEBUG_MAX]._function = function;
		_debugs[idx % dfDEBUG_MAX]._pPrevTop = pPrevTop;
		_debugs[idx % dfDEBUG_MAX]._pNewTop = pNewTop;
	}

	DebugData _debugs[dfDEBUG_MAX];
	long _idx = -1;
	*/

public:
	inline void Push(T data)
	{
		StackNode* pNewTop = new StackNode(data);

		for (;;)
		{
			StackNode* pPrevTop = (StackNode*) _pTop;
			pNewTop->_next = pPrevTop;
			if (InterlockedCompareExchangePointer(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				return;
			}
		}
	}

	T Pop()
	{
		for (;;)
		{
			StackNode* pPrevTop = (StackNode*) _pTop;
			StackNode* pNewTop = pPrevTop->_next;
			if (InterlockedCompareExchangePointer(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				T data = pPrevTop->_data;
				delete pPrevTop;
				return data;
			}
		}
	}

private:
	volatile PVOID _pTop = nullptr;

};
