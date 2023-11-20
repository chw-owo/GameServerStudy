#pragma once
#include "LockFreePool.h"
#include <windows.h>
#include <process.h>
#include <vector>
using namespace std;
#define __LOCKFREESTACK_DEBUG__

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
	void Push(DATA data)
	{
		StackNode* pNewNode = _pPool->Alloc();
		pNewNode->_data = data;

		// For Protect ABA
		__int64 key = InterlockedIncrement64(&_key);
		key <<= __USESIZE_64BIT__;
		__int64 pNewTop = (__int64)pNewNode;
		pNewTop &= _addressMask;
		unsigned __int64 tmp = pNewTop;
		pNewTop |= key;

		unsigned int keyIdx = (key >> __USESIZE_64BIT__);
		long valueIdx = InterlockedIncrement(&_keyDebugIdx[keyIdx]);
		_keyDebugArray[keyIdx][(valueIdx % dfVALUE_MAX)]._idx = valueIdx;
		_keyDebugArray[keyIdx][(valueIdx % dfVALUE_MAX)]._address = tmp;
		_keyDebugArray[keyIdx][(valueIdx % dfVALUE_MAX)]._data = data;

		for (;;)
		{
#ifdef __LOCKFREESTACK_DEBUG__

			DATA compVal = -1;
			__int64 pPrevTop = _pTop;
			if (pPrevTop != NULL)
				compVal = ((StackNode*)(pPrevTop & _addressMask))->_data;
			
			pNewNode->_next = pPrevTop;
			DATA exchVal = data;

			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(0, (unsigned __int64)pNewTop, (unsigned __int64)pPrevTop, exchVal, compVal);
				return;
			}

#else
			__int64 pPrevTop = _pTop;
			pNewNode->_next = pPrevTop;

			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				return;
			}
#endif
		}
	}

	void Pop()
	{
		for (;;)
		{
#ifdef __LOCKFREESTACK_DEBUG__

			__int64 pPrevTop = _pTop;
			DATA compVal = ((StackNode*)(pPrevTop & _addressMask))->_data; // 지금 테스트 환경에서는 pTop이 늘 존재

			DATA exchVal = -1;
			__int64 pNextTop = ((StackNode*)(pPrevTop & _addressMask))->_next;
			if (pNextTop != NULL)
				exchVal = ((StackNode*)(pNextTop & _addressMask))->_data;

			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(1, (unsigned __int64)pNextTop, (unsigned __int64)pPrevTop, exchVal, compVal);
				_pPool->Free((StackNode*)(pPrevTop & _addressMask));
				return;
			}
#else

			__int64 pPrevTop = _pTop;
			__int64 pNextTop = ((StackNode*)(pPrevTop & _addressMask))->_next;

			if (InterlockedCompareExchange64(&_pTop, pNext, pPrevTop) == pPrevTop)
			{
				_pPool->Free((StackNode*)(pPrevTop & _addressMask));
				return;
			}
#endif
		}
	}

#ifdef __LOCKFREESTACK_DEBUG__
private:
	class StackDebugData
	{
		friend CLockFreeStack;

	private:
		StackDebugData() : _idx(-1), _threadID(-1), _line(-1), 
			_compKey(-1), _exchKey(-1), _compAddress(-1), _exchAddress(-1) {}

		void SetData(int idx, int threadID, int line, int exchKey, int compKey,
			__int64 exchAddress, __int64 compAddress, DATA exchVal, DATA compVal)
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

			__faststorefence();
		}

	private:
		int _idx;
		int _threadID;
		int _line;
		int _compKey;
		int _exchKey;
		__int64 _compAddress;
		__int64 _exchAddress;
		DATA _compVal;
		DATA _exchVal;
	};

private:
#define dfSTACK_DEBUG_MAX 10000
	StackDebugData _stackDebugArray[dfSTACK_DEBUG_MAX];
	volatile long _stackDebugIdx = -1;

#define __BREAK_IN_STACKLOG__
private:
	inline void LeaveLog(int line, unsigned __int64 exchange, 
		unsigned __int64 comperand, DATA exchVal, DATA compVal)
	{
		LONG idx = InterlockedIncrement(&_stackDebugIdx);

		int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
		int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
		__int64 exchAddress = exchange & _addressMask;
		__int64 compAddress = comperand & _addressMask;

		_stackDebugArray[idx % dfSTACK_DEBUG_MAX].SetData( 
			idx, GetCurrentThreadId(), line, exchKey, compKey, 
			exchAddress, compAddress, exchVal, compVal);

#ifdef __BREAK_IN_STACKLOG__

		if (idx < 200) return;
		int checkIdx1 = (idx - 200) % dfSTACK_DEBUG_MAX;
		int checkIdx2 = (idx - 199) % dfSTACK_DEBUG_MAX;

		__int64 addr2 = _stackDebugArray[checkIdx2]._compAddress;
		int key2 = _stackDebugArray[checkIdx2]._compKey;
		DATA val2 = _stackDebugArray[checkIdx2]._compVal;

		__int64 addr1 = _stackDebugArray[checkIdx1]._exchAddress;
		int key1 = _stackDebugArray[checkIdx1]._exchKey;
		DATA val1 = _stackDebugArray[checkIdx1]._exchVal;

		if ((val1 != val2) && (key1 == key2) && (addr1 == addr2))
		{
			::printf("Stack[%d]: %d != %d, %d == %d, %lld == %lld\n",
				checkIdx2, val1, val2, key1, key2, addr1, addr2);
			__debugbreak();
		}
#endif
	}

private:
#define dfKEY_MAX 131071
#define dfVALUE_MAX 100
	struct KeyDebugData
	{
		long _idx = 0;
		unsigned __int64 _address = 0;
		unsigned __int64 _realAddress = 0;
		DATA _data;
	};

	KeyDebugData _keyDebugArray[dfKEY_MAX][dfVALUE_MAX];
	volatile long _keyDebugIdx[dfKEY_MAX] = { 0, };

#endif
};

