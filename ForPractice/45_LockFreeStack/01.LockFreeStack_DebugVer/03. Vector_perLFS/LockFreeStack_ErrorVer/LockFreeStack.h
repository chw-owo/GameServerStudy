#pragma once
#include "DebugQ.h"
#include <windows.h>
#include <process.h>
#include <vector>
using namespace std;

template<typename T>
class LockFreeStack
{
private:
	struct Node
	{
		T _data;
		Node* _next;
	};

private:
	int _size = 0;
	Node* _pTop = nullptr;

public:
	LockFreeStack()
	{
		InitializeCriticalSection(&_cs);
		_dataArray.reserve(1500000);
	}

	void Push(T data)
	{
		Node* pNew = new Node;
		pNew->_data = data;

		for(;;)
		{
			Node* pTopLocal = _pTop;
			pNew->_next = pTopLocal;

			EnterCriticalSection(&_cs);
			if (InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pNew, (LONG64)pTopLocal))
			{
				DebugData* data = new DebugData(
					GetCurrentThreadId(), 0, (__int64)pNew, (__int64)pTopLocal, (__int64)_pTop);
				_dataArray.push_back(data);
				LeaveCriticalSection(&_cs);
				break;
			}
			LeaveCriticalSection(&_cs);
		} 
	}

	void Pop()
	{
		for(;;)
		{
			Node* pTopLocal = _pTop;
			Node* pNext = pTopLocal->_next;

			EnterCriticalSection(&_cs);
			if (InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pNext, (LONG64)pTopLocal))
			{
				DebugData* data = new DebugData(
					GetCurrentThreadId(), 1, (__int64)pNext, (__int64)pTopLocal, (__int64)_pTop);
				_dataArray.push_back(data);
				LeaveCriticalSection(&_cs);
				delete pTopLocal;
				break;
			}
			LeaveCriticalSection(&_cs);
		} 
	}

	// For Debug ========================================================

private:
	class DebugData
	{
		friend LockFreeStack;

	private:
		DebugData() :
			_threadID(-1), _line(-1), 
			_exchange(-1), _comperand(-1), _top(-1) {}

		DebugData(int threadID, int line, 
			__int64 exchange, __int64 comperand, __int64 top)
			: _threadID(threadID), _line(line), 
			_exchange(exchange), _comperand(comperand), _top(top) {}

	private:
		int _threadID;
		int _line;
		__int64 _comperand;
		__int64 _exchange;
		__int64 _top;
	};

private: 
	CRITICAL_SECTION _cs;
	vector<DebugData*> _dataArray;
};



