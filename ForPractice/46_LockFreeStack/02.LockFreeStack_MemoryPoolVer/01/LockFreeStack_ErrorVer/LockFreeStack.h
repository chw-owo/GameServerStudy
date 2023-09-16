#pragma once
#include "LockFreePool.h"
#include <windows.h>
#include <process.h>
#include <vector>
using namespace std;

template<typename DATA>
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
		DATA _data = 0;
		StackNode* _next = nullptr;
	};

public:
	CLockFreeStack()
	{
		_pPool = new CLockFreePool<StackNode>(0, false);
	}

private:
	StackNode* _pTop = nullptr;
	CLockFreePool<StackNode>* _pPool = nullptr;

public:
	void Push(DATA data)
	{
		StackNode* pNew = _pPool->Alloc();
		pNew->_data = data;

		for(;;)
		{
			StackNode* pTopLocal = _pTop;
			pNew->_next = pTopLocal;
			
			DATA compVal;
			DATA exchVal = data;
			if (pTopLocal != nullptr) compVal = pTopLocal->_data;
			
			if (InterlockedCompareExchange64((LONG64*)&_pTop, 
				(LONG64)pNew, (LONG64)pTopLocal) == (LONG64)pTopLocal)
			{		
				LeaveLog(0, (LONG64)pNew, (LONG64)pTopLocal, exchVal, compVal);
				return;
			}
		} 
	}

	void Pop()
	{
		for(;;)
		{
			StackNode* pTopLocal = _pTop;
			StackNode* pNext = pTopLocal->_next;

			DATA exchVal;
			DATA compVal = pTopLocal->_data; // 지금 테스트 환경에서는 pTop이 늘 존재
			if (pNext != nullptr) exchVal = pNext->_data;

			if (InterlockedCompareExchange64((LONG64*)&_pTop, 
				(LONG64)pNext, (LONG64)pTopLocal) == (LONG64)pTopLocal)
			{
				LeaveLog(1, (LONG64)pNext, (LONG64)pTopLocal, exchVal, compVal);
				_pPool->Free(pTopLocal);
				return;
			}
		} 
	}

	// For Debug ========================================================

private:
	class StackDebugData
	{
		friend CLockFreeStack;

	private:
		StackDebugData() : _idx(-1), _threadID(-1), _line(-1), _exchange(-1), _comperand(-1){}
		void SetData(int idx, int threadID, int line,  __int64 exchange, __int64 comperand, DATA exchVal, DATA compVal)
		{
			_idx = idx;
			_threadID = threadID;
			_line = line;
			_exchange = exchange;
			_comperand = comperand;
			_exchVal = exchVal;
			_compVal = compVal;
		}

	private:
		int _idx;
		int _threadID;
		int _line;
		__int64 _comperand;
		__int64 _exchange;
		DATA _compVal;
		DATA _exchVal;
	};

private:
#define dfSTACK_DEBUG_MAX 1000
	inline void LeaveLog(int line, __int64 exchange, __int64 comperand, DATA exchVal, DATA compVal)
	{
		LONG idx = InterlockedIncrement(&_stackDebugIdx);

		_stackDebugArray[idx % dfSTACK_DEBUG_MAX].SetData(
			idx, GetCurrentThreadId(), line, exchange, comperand, exchVal, compVal);

		// TO-DO... 값이 분명 같은데 다르다고 나옴 왜!!!!
		if ((_stackDebugArray[(idx - 1) % dfSTACK_DEBUG_MAX]._exchVal != compVal) &&
			(_stackDebugArray[(idx - 1) % dfSTACK_DEBUG_MAX]._exchange == comperand))
		{
			::printf("Stack[%d]: %d != %d\n", idx % dfSTACK_DEBUG_MAX,
				_stackDebugArray[(idx - 1) % dfSTACK_DEBUG_MAX]._exchVal, compVal);
			__debugbreak();
		}

	}

private: 
	StackDebugData _stackDebugArray[dfSTACK_DEBUG_MAX];
	volatile long _stackDebugIdx = -1;

};



