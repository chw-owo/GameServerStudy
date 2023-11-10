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

public:
	static CTlsPool<StackNode>* _pStackPool;

private: // For protect ABA
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

private:
	// 17 bit key + 47 bit address
	__int64 CreateAddress(QueueNode* node)
	{
		unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
		key <<= __ADDRESS_BIT__;
		__int64 address = (__int64)node;
		address &= _addressMask;
		address |= key;

		return address;
	}

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
		__int64 newTop = CreateAddress(pNewNode);

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
				StackNode* pPrevNode = (StackNode*)(pPrevTop & _addressMask);
				T data = pPrevNode->_data;
				_pPool->Free(pPrevNode);
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

		void SetData(int idx, int threadID, int line, 
			int exchKey, int compKey, __int64 exchAddress, __int64 compAddress)
		{
			_idx = idx;
			_threadID = threadID;
			_line = line;
			_exchKey = exchKey;			
			_compKey = compKey; 
			_exchAddress = exchAddress;
			_compAddress = compAddress;
		}

	private:
		int _idx;
		int _threadID;
		int _line;
		int _compKey;
		int _exchKey;
		__int64 _compAddress;
		__int64 _exchAddress;
	};

};

template<typename T>
CTlsPool<typename CLockFreeStack<T>::StackNode>* CLockFreeStack<T>::_pStackPool = new CTlsPool<CLockFreeStack<T>::StackNode>(0, false);