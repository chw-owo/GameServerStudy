#pragma once
#include <new.h>
#include <windows.h>

#define  __OBJECT_POOL__
// #define __OBJECTPOOL_DEBUG__

template <class T>
class CLockFreePool
{
private:

#ifdef __OBJECTPOOL_DEBUG__

	unsigned char _objectPoolID = 0;
	struct PoolNode
	{
		__int64 head;
		T _data;
		__int64 _next;
	};

#else
	struct PoolNode
	{
		T _data;
		__int64 _next = NULL;
	};
#endif

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
	__int64 _pTop = NULL;

#ifdef __OBJECTPOOL_DEBUG__
public:
	int		GetCapacityCount(void) { return _capacity; }
	int		GetUseCount(void) { return _useCount; }

private:
	int _capacity;
	int _useCount;
	unsigned char _poolID;
#endif

private:
	class PoolDebugData
	{
		friend CLockFreePool;

	private:
		PoolDebugData() : _idx(-1), _threadID(-1), _line(-1),
			_compKey(-1), _exchKey(-1), _compAddress(-1), _exchAddress(-1) {}

		void SetData(int idx, int threadID, int line, int exchKey, int compKey, __int64 exchAddress, __int64 compAddress)
		{
			_exchKey = exchKey;
			_exchAddress = exchAddress;

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
	};

private:
#define dfPOOL_DEBUG_MAX 1000
	PoolDebugData _poolDebugArray[dfPOOL_DEBUG_MAX];
	volatile long _poolDebugIdx = -1;

private:
	inline void LeaveLog(int line, unsigned __int64 exchange, unsigned __int64 comperand)
	{
		LONG idx = InterlockedIncrement(&_poolDebugIdx);

		int exchKey = (exchange >> __USESIZE_64BIT__) & _keyMask;
		int compKey = (comperand >> __USESIZE_64BIT__) & _keyMask;
		__int64 exchAddress = exchange & _addressMask;
		__int64 compAddress = comperand & _addressMask;

		_poolDebugArray[idx % dfPOOL_DEBUG_MAX].SetData(
			idx, GetCurrentThreadId(), line, exchKey, compKey, exchAddress, compAddress);
	}
};

template<class T>
template<typename... Types>
CLockFreePool<T>::CLockFreePool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pTop(NULL)
{
	_keyMask = 0b11111111111111111;
	_addressMask = 0b11111111111111111;
	_addressMask <<= __USESIZE_64BIT__;
	_addressMask = ~_addressMask;

#ifdef __OBJECTPOOL_DEBUG__

	_capacity = _blockNum;
	_useCount = 0;
	_poolID = _objectPoolID;
	_objectPoolID++;
#endif

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

template<class T>
CLockFreePool<T>::~CLockFreePool()
{
	if (_pTop == NULL)
		return;

	if (_placementNew)
	{
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
		while (((PoolNode*)(_pTop & _addressMask))->_next != NULL)
		{
			__int64 next = ((PoolNode*)(_pTop & _addressMask))->_next;
			free(((PoolNode*)(_pTop & _addressMask)));
			_pTop = next;
		}
		free(((PoolNode*)(_pTop & _addressMask)));
	}
	else
	{
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다

		while (((PoolNode*)(_pTop & _addressMask))->_next != NULL)
		{
			__int64 next = ((PoolNode*)(_pTop & _addressMask))->_next;
			(((PoolNode*)(_pTop & _addressMask))->_data).~T();
			free(((PoolNode*)(_pTop & _addressMask)));
			_pTop = next;
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
			if (pPrevTop == NULL)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
				PoolNode* pNewNode = (PoolNode*)malloc(sizeof(PoolNode));
				new (&(pNewNode->_data)) T(args...);

#ifdef __OBJECTPOOL_DEBUG__

				_capacity++;
				_useCount++;

				__int64 code = 0;
				code |= (__int64)_poolID << (3 * 3);
				code |= 0777 & (__int64)(&(pNewNode->_data));

				pNewNode->head = code;
				pNewNode->_next = code;

#endif
				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next;
			if (InterlockedCompareExchange64(&_pTop, pNext, pPrevTop) == pPrevTop)
			{
				LeaveLog(1, (unsigned __int64)pNextTop, (unsigned __int64)pPrevTop);
				new (&(((PoolNode*)(pPrevTop & _addressMask))->_data)) T(args...);

#ifdef __OBJECTPOOL_DEBUG__
				_useCount++;
				__int64 code = 0;
				code |= (__int64)_poolID << (3 * 3);
				code |= 0777 & (__int64)(&(((PoolNode*)(pPrevTop & _addressMask))->_data));

				((PoolNode*)(pPrevTop & _addressMask))->head = code;
				((PoolNode*)(pPrevTop & _addressMask))->_next = code;
#endif
				return &(((PoolNode*)(pPrevTop & _addressMask))->_data);
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
#ifdef __OBJECTPOOL_DEBUG__

				_capacity++;
				_useCount++;

				__int64 code = 0;
				code |= (__int64)_poolID << (3 * 3);
				code |= 0777 & (__int64)(&(pNewNode->_data));

				pNewNode->head = code;
				pNewNode->_next = code;

#endif
				return &(pNewNode->_data);
			}

			__int64 pNextTop = ((PoolNode*)(pPrevTop & _addressMask))->_next;
			if (InterlockedCompareExchange64(&_pTop, pNext, pPrevTop) == pPrevTop)
			{
				LeaveLog(1, (unsigned __int64)pNextTop, (unsigned __int64)pPrevTop, exchVal, compVal);
#ifdef __OBJECTPOOL_DEBUG__
				_useCount++;
				__int64 code = 0;
				code |= (__int64)_poolID << (3 * 3);
				code |= 0777 & (__int64)(&(((PoolNode*)(pPrevTop & _addressMask))->_data));

				((PoolNode*)(pPrevTop & _addressMask))->head = code;
				((PoolNode*)(pPrevTop & _addressMask))->_next = code;
#endif
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
	unsigned __int64 tmp = pNewTop;
	pNewTop |= key;

	if (_placementNew)
	{
		// Data의 소멸자를 호출한 후 _pTop에 push한다

#ifdef __OBJECTPOOL_DEBUG__

		_useCount--;

		__int64 code = 0;
		code |= (__int64)_poolID << (3 * 3);
		code |= 0777 & (__int64)pData;

		__int64 offset = (__int64)(&(((PoolNode*)nullptr)->_data));
		PoolNode* pNode = (PoolNode*)((__int64)pData - offset);

		if (pNode->head != code || pNode->_next != code)
		{
			return false;
		}

#endif
		pData->~T();

		for (;;)
		{
			__int64 pPrevTop = _pTop;
			((PoolNode*)pData)->_next = pPrevTop;

			if (InterlockedCompareExchange64(&_pTop, pData, pPrevTop) == pPrevTop)
			{
				LeaveLog(0, (unsigned __int64)pNewTop, (unsigned __int64)pPrevTop);
				return true;
			}
		}
	}
	else
	{
		// Data의 소멸자를 호출하지 않고 _pTop에 push한다

#ifdef __OBJECTPOOL_DEBUG__

		_useCount--;

		__int64 code = 0;
		code |= (__int64)_poolID << (3 * 3);
		code |= 0777 & (__int64)pData;

		__int64 offset = (__int64)(&(((PoolNode*)nullptr)->_data));
		PoolNode* pNode = (PoolNode*)((__int64)pData - offset);

		if (pNode->head != code || pNode->_next != code)
		{
			return false;
		}

#endif
		for (;;)
		{
			__int64 pPrevTop = _pTop;
			((PoolNode*)pData)->_next = pPrevTop;

			if (InterlockedCompareExchange64(&_pTop, pNewTop, pPrevTop) == pPrevTop)
			{
				LeaveLog(0, (unsigned __int64)pNewTop, (unsigned __int64)pPrevTop);
				return true;
			}
		}
	}
	return false;
}


