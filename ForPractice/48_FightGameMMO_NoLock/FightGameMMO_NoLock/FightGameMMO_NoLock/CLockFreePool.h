#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <new.h>
#include <windows.h>

template <class T>
class CLockFreePool
{
private:
	struct PoolNode
	{
		T _data;
		__int64 _next = 0;
	};

private: // For protect ABA
#define __USESIZE_64BIT__ 47
	unsigned __int64 _addressMask = 0;
	unsigned int _keyMask = 0;
	volatile __int64 _key = 0;

public:
	template<typename... Types>
	CLockFreePool(int blockNum, bool placementNew, Types... args);
	virtual	~CLockFreePool();

public:
	template<typename... Types>
	inline T* Alloc(Types... args);
	inline bool Free(T* pData);

private:
	bool _placementNew;
	int _blockNum;
	__int64 _pTop = 0;

private: // For Log
	class PoolDebugData
	{
		friend CLockFreePool;

	private:
		PoolDebugData() : _idx(-1), _threadID(-1), _line(-1), _size(-1), 
			_compKey(-1), _exchKey(-1), _realKey(-1), _compAddress(-1), _exchAddress(-1), _realAddress(-1) {}

		void SetData(int idx, int threadID, int line, int size,
			int exchKey, __int64 exchAddress,
			int compKey, __int64 compAddress,
			int realKey, __int64 realAddress,
			T* data)
		{
			_exchKey = exchKey;
			_exchAddress = exchAddress;

			_compKey = compKey;
			_compAddress = compAddress;

			_realKey = realKey;
			_realAddress = realAddress;

			_idx = idx;
			_threadID = threadID;
			_line = line;
			_size = size;
			_data = data;
		}

	private:
		int _idx;
		int _threadID;
		int _line;
		int _size;

		int _compKey;
		__int64 _compAddress;

		int _exchKey;
		__int64 _exchAddress;

		int _realKey;
		__int64 _realAddress;

		T* _data;
	};

private:
#define dfPOOL_DEBUG_MAX 1000

	inline void LeaveLog(int line, int size,
		unsigned __int64 exchange, unsigned __int64 comperand, unsigned __int64 real, T* data)
	{
		LONG idx = InterlockedIncrement(&_poolDebugIdx);

		int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
		int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
		int realKey = (real >> __USESIZE_64BIT__) & _keyMask;
		__int64 exchAddress = exchange & _addressMask;
		__int64 compAddress = comperand & _addressMask;
		__int64 realAddress = real & _addressMask;

		_poolDebugArray[idx % dfPOOL_DEBUG_MAX].SetData(idx, GetCurrentThreadId(), line, size,
			exchKey, exchAddress, compKey, compAddress, realKey, realAddress, data);
	}

	PoolDebugData _poolDebugArray[dfPOOL_DEBUG_MAX];
	volatile long _poolDebugIdx = -1;
};

template<class T>
template<typename... Types>
CLockFreePool<T>::CLockFreePool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pTop(0)
{
	_keyMask = 0b11111111111111111;
	_addressMask = 0b11111111111111111;
	_addressMask <<= __USESIZE_64BIT__;
	_addressMask = ~_addressMask;

	if (_blockNum <= 0)
		return;

	if (_placementNew)
	{
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다

		PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));

		// For Protect ABA
		unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
		key <<= __USESIZE_64BIT__;
		__int64 newTop = (__int64)pNewNode;
		newTop &= _addressMask;
		newTop |= key;

		_pTop = newTop;

		for (int i = 1; i < _blockNum; i++)
		{
			pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
			pNewNode->_next = _pTop;

			key = (unsigned __int64)InterlockedIncrement64(&_key);
			key <<= __USESIZE_64BIT__;
			__int64 newTop = (__int64)pNewNode;
			newTop &= _addressMask;
			newTop |= key;

			_pTop = newTop;
		}
	}
	else
	{
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다

		PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
		_pTop = (__int64)pNewNode;

		for (int i = 1; i < _blockNum; i++)
		{
			new (&(((PoolNode*)_pTop)->_data)) T(args...);
			PoolNode* p = (PoolNode*)malloc(sizeof(PoolNode));
			p->_next = _pTop;
			_pTop = (__int64)p;
		}
		new (&(((PoolNode*)_pTop)->_data)) T(args...);
	}
}

// TO-DO: 소멸자 Error
template<class T>
CLockFreePool<T>::~CLockFreePool()
{
	if (_pTop == NULL)
		return;

	if (_placementNew)
	{
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
		__int64 next = ((PoolNode*)(_pTop & _addressMask))->_next;
		while ((next & _addressMask) != 0)
		{
			free(((PoolNode*)(_pTop & _addressMask)));
			_pTop = next;
			next = ((PoolNode*)(_pTop & _addressMask))->_next;
		}
		free(((PoolNode*)(_pTop & _addressMask)));
	}
	else
	{
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다
		int idx = 0;
		__int64 next = ((PoolNode*)(_pTop & _addressMask))->_next;
		while ((next & _addressMask) != 0)
		{
			idx++;
			(((PoolNode*)(_pTop & _addressMask))->_data).~T();
			free(((PoolNode*)(_pTop & _addressMask)));
			_pTop = next;
			next = ((PoolNode*)(_pTop & _addressMask))->_next;
		}
		(((PoolNode*)(_pTop & _addressMask))->_data).~T();
		free(((PoolNode*)(_pTop & _addressMask)));
	}
}

template<class T>
template<typename... Types>
T* CLockFreePool<T>::Alloc(Types... args) // Pop
{
	if (_placementNew)
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == 0)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
				
				PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
				new (&(pNewNode->_data)) T(args...);

				LeaveLog(0, 0, 0, 0, 0, &(pNewNode->_data));

				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next; 
			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				T* data = &(((PoolNode*)(pPrevTop & _addressMask))->_data);
				new (data) T(args...);

				LeaveLog(1, 0, pNextTop, pPrevTop, 0, data);

				return data;
			}
		}
	}
	else
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출하지 않는다
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			if (pPrevTop == NULL)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
				
				PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
				new (&(pNewNode->_data)) T(args...);

				LeaveLog(2, 0, 0, 0, 0, &(pNewNode->_data));

				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next;
			if (InterlockedCompareExchange64(&_pTop, pNextTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(3, 0, pNextTop, pPrevTop, 0, &(((PoolNode*)(pPrevTop & _addressMask))->_data));

				return &(((PoolNode*)(pPrevTop & _addressMask))->_data);
			}
		}
	}
	return nullptr;
}

template<class T>
bool CLockFreePool<T>::Free(T* pData) // Push
{
	// For Protect ABA
	unsigned __int64 key = (unsigned __int64)InterlockedIncrement64(&_key);
	key <<= __USESIZE_64BIT__;
	__int64 pNewTop = (__int64)pData;
	pNewTop &= _addressMask;
	pNewTop |= key;

	if (_placementNew)
	{
		// Data의 소멸자를 호출한 후 _pTop에 push한다
		pData->~T();

		for (;;)
		{
			__int64 pPrevTop = _pTop;
			((PoolNode*)pData)->_next = pPrevTop;
			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(4, 0, pNewTop, pPrevTop, 0, pData);

				return true;
			}
		}
	}
	else
	{
		// Data의 소멸자를 호출하지 않고 _pTop에 push한다
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			((PoolNode*)pData)->_next = pPrevTop;
			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(5, 0, pNewTop, pPrevTop, 0, pData);

				return true;
			}
		}
	}
	return false;
}