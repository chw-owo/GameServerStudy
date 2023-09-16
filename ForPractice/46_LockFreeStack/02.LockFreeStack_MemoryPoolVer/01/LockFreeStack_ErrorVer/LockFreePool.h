#pragma once
#ifndef  __OBJECT_POOL__
#define  __OBJECT_POOL__
#include <new.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#ifdef __OBJECT_POOL_DEBUG__

#endif

template <class DATA>
class CLockFreePool
{
private:

#ifdef __OBJECT_POOL_DEBUG__
	unsigned char _objectPoolID = 0;

	struct PoolNode
	{
		size_t head;
		DATA _data;
		size_t _tail;
	};

#else
	struct PoolNode
	{
		DATA _data;
		size_t _tail = nullptr;
	};
#endif

public:
	template<typename... Types>
	CLockFreePool(int blockNum, bool placementNew, Types... args);
	virtual	~CLockFreePool();

public:
	template<typename... Types>
	inline DATA* Alloc(Types... args);
	inline bool Free(DATA* pData);

private:
	bool _placementNew;
	int _blockNum;
	PoolNode* _pFreeNode = nullptr;

#ifdef __OBJECT_POOL_DEBUG__
public:
	int		GetCapacityCount(void) { return _capacity; }
	int		GetUseCount(void) { return _useCount; }

private:
	int _capacity;
	int _useCount;
	unsigned char _poolID;
#endif

	// For Multithread Debug
private:
	class PoolDebugData
	{
		friend CLockFreePool;

	private:
		PoolDebugData() : _idx(-1), _threadID(-1), _line(-1), _exchange(-1), _comperand(-1) {}
		void SetData(int idx, int threadID, int line, __int64 exchange, __int64 comperand, DATA exchVal, DATA compVal)
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
#define dfPOOL_DEBUG_MAX 1000
	inline void LeaveLog(int line,__int64 exchange, __int64 comperand, DATA exchVal, DATA compVal)
	{
		LONG idx = InterlockedIncrement(&_poolDebugIdx);

		_poolDebugArray[idx % dfPOOL_DEBUG_MAX].SetData(
			idx, GetCurrentThreadId(), line, exchange, comperand, exchVal, compVal);

		if ((_poolDebugArray[(idx - 1) % dfPOOL_DEBUG_MAX]._exchVal != compVal) &&
			(_poolDebugArray[(idx - 1) % dfPOOL_DEBUG_MAX]._exchange == comperand))
		{
			::printf("Pool[%d]: %d != %d\n", idx % dfPOOL_DEBUG_MAX, 
				_poolDebugArray[(idx - 1) % dfPOOL_DEBUG_MAX]._exchVal, compVal);
			__debugbreak();
		}
	}

private:
	PoolDebugData _poolDebugArray[dfPOOL_DEBUG_MAX];
	volatile long _poolDebugIdx = -1;
};

template<class DATA>
template<typename... Types>
CLockFreePool<DATA>::CLockFreePool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pFreeNode(nullptr)
{

#ifdef __OBJECT_POOL_DEBUG__

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

		_pFreeNode = (PoolNode*)malloc(sizeof(PoolNode));
		_pFreeNode->_tail = (size_t)nullptr;
		for (int i = 1; i < _blockNum; i++)
		{
			PoolNode* p = (PoolNode*)malloc(sizeof(PoolNode));
			p->_tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
	}
	else
	{
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다

		_pFreeNode = (PoolNode*)malloc(sizeof(PoolNode));
		_pFreeNode->_tail = (size_t)nullptr;
		for (int i = 1; i < _blockNum; i++)
		{
			new (&(_pFreeNode->_data)) DATA(args...);
			PoolNode* p = (PoolNode*)malloc(sizeof(PoolNode));
			p->_tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
		new (&(_pFreeNode->_data)) DATA(args...);
	}
}

template<class DATA>
CLockFreePool<DATA>::~CLockFreePool()
{
#ifdef __OBJECT_POOL_DEBUG__

	if (_useCount != 0)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: There is Unfree Data\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"ERROR", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: There is Unfree Data\n",
			_T(__FUNCTION__), __LINE__);
	}

#endif

	if (_pFreeNode == nullptr)
		return;

	if (_placementNew)
	{
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다

		while (_pFreeNode->_tail != (size_t)nullptr)
		{
			size_t next = _pFreeNode->_tail;
			free(_pFreeNode);
			_pFreeNode = (PoolNode*)next;
		}
		free(_pFreeNode);
	}
	else
	{
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다

		while (_pFreeNode->_tail != (size_t)nullptr)
		{
			size_t next = _pFreeNode->_tail;
			(_pFreeNode->_data).~DATA();
			free(_pFreeNode);
			_pFreeNode = (PoolNode*)next;
		}
		(_pFreeNode->_data).~DATA();
		free(_pFreeNode);
	}
}

template<class DATA>
template<typename... Types>
DATA* CLockFreePool<DATA>::Alloc(Types... args) // Pop
{
	if (_placementNew)
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다
		for (;;)
		{
			PoolNode* pTopLocal = _pFreeNode;

			if (pTopLocal == nullptr)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
				PoolNode* pNew = (PoolNode*)malloc(sizeof(PoolNode));
				new (&(pNew->_data)) DATA(args...);

#ifdef __OBJECT_POOL_DEBUG__

				_capacity++;
				_useCount++;

				size_t code = 0;
				code |= (size_t)_poolID << (3 * 3);
				code |= 0777 & (size_t)(&(pNew->_data));

				pNew->head = code;
				pNew->_tail = code;

#endif
				return &(pNew->_data);
			}

			PoolNode* pNext = (PoolNode*)pTopLocal->_tail;

			DATA exchVal;
			DATA compVal = pTopLocal->_data; 
			if (pNext != nullptr) exchVal = pNext->_data;

			if (InterlockedCompareExchange64((LONG64*)&_pFreeNode, 
				(LONG64)pNext, (LONG64)pTopLocal) == (LONG64)pTopLocal)
			{
				LeaveLog(0, (LONG64)pNext, (LONG64)pTopLocal, exchVal, compVal);
				new (&(pTopLocal->_data)) DATA(args...);

#ifdef __OBJECT_POOL_DEBUG__
				_useCount++;
				size_t code = 0;
				code |= (size_t)_poolID << (3 * 3);
				code |= 0777 & (size_t)(&(pTopLocal->_data));

				pTopLocal->head = code;
				pTopLocal->_tail = code;
#endif
				return &(pTopLocal->_data);
			}
		}
	}
	else
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출하지 않는다
		for (;;)
		{
			PoolNode* pTopLocal = _pFreeNode;

			if (pTopLocal == nullptr)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
				PoolNode* pNew = (PoolNode*)malloc(sizeof(PoolNode));

#ifdef __OBJECT_POOL_DEBUG__

				_capacity++;
				_useCount++;

				size_t code = 0;
				code |= (size_t)_poolID << (3 * 3);
				code |= 0777 & (size_t)(&(pNew->_data));

				pNew->head = code;
				pNew->_tail = code;

#endif
				return &(pNew->_data);
			}
			
			PoolNode* pNext = (PoolNode*)pTopLocal->_tail;

			DATA exchVal;
			DATA compVal = pTopLocal->_data;
			if (pNext != nullptr) exchVal = pNext->_data;

			if (InterlockedCompareExchange64((LONG64*)&_pFreeNode, 
				(LONG64)pNext, (LONG64)pTopLocal) == (LONG64)pTopLocal)
			{
				LeaveLog(0, (LONG64)pNext, (LONG64)pTopLocal, exchVal, compVal);

#ifdef __OBJECT_POOL_DEBUG__
				_useCount++;
				size_t code = 0;
				code |= (size_t)_poolID << (3 * 3);
				code |= 0777 & (size_t)(&(pTopLocal->_data));

				pTopLocal->head = code;
				pTopLocal->_tail = code;
#endif
				return &(pTopLocal->_data);
			}
		}
	}
	return nullptr;
}

template<class DATA>
bool CLockFreePool<DATA>::Free(DATA* pData) // Push
{
	if (_placementNew)
	{
		// Data의 소멸자를 호출한 후 _pFreeNode에 push한다

#ifdef __OBJECT_POOL_DEBUG__

		_useCount--;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)pData;


		size_t offset = (size_t)(&(((PoolNode*)nullptr)->_data));
		PoolNode* pNode = (PoolNode*)((size_t)pData - offset);

		if (pNode->head != code || pNode->_tail != code)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Code is Diffrent. code %o, head %o, _tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->_tail);

			::wprintf(L"%s[%d]: Code is Diffrent. code %o, head %o, _tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->_tail);

			return false;
		}

		pData->~DATA();
		pNode->_tail = (size_t)_pFreeNode;
		_pFreeNode = pNode;
		return true;

#else
		pData->~DATA();

		for (;;)
		{
			PoolNode* pTopLocal = _pFreeNode;
			((PoolNode*)pData)->_tail = (size_t)pTopLocal;

			DATA compVal;
			DATA exchVal = ((PoolNode*)pData)->_data;
			if (pTopLocal != nullptr) compVal = pTopLocal->_data;

			if (InterlockedCompareExchange64((LONG64*)&_pFreeNode, 
				(LONG64)pData, (LONG64)pTopLocal) == (LONG64)pTopLocal)
			{
				LeaveLog(1, (LONG64)pData, (LONG64)pTopLocal, exchVal, compVal);
				return true;
			}
		}
#endif
	}
	else
	{
		// Data의 소멸자를 호출하지 않고 _pFreeNode에 push한다

#ifdef __OBJECT_POOL_DEBUG__

		_useCount--;

		size_t code = 0;
		code |= (size_t)_poolID << (3 * 3);
		code |= 0777 & (size_t)pData;

		size_t offset = (size_t)(&(((PoolNode*)nullptr)->_data));
		PoolNode* pNode = (PoolNode*)((size_t)pData - offset);

		if (pNode->head != code || pNode->_tail != code)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Code is Different. code %o, head %o, _tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->_tail);

			::wprintf(L"%s[%d]: Code is Different. code %o, head %o, _tail %o\n",
				_T(__FUNCTION__), __LINE__, code, pNode->head, pNode->_tail);

			return false;
		}

		pNode->_tail = (size_t)_pFreeNode;
		_pFreeNode = pNode;

		return true;
#else
		for (;;)
		{
			PoolNode* pTopLocal = _pFreeNode;
			((PoolNode*)pData)->_tail = (size_t)pTopLocal;

			DATA compVal;
			DATA exchVal = ((PoolNode*)pData)->_data;
			if (pTopLocal != nullptr) compVal = pTopLocal->_data;

			if (InterlockedCompareExchange64((LONG64*)&_pFreeNode, 
				(LONG64)pData, (LONG64)pTopLocal) == (LONG64)pTopLocal)
			{
				LeaveLog(1, (LONG64)pData, (LONG64)pTopLocal, exchVal, compVal);
				return true;
			}
		}
#endif
	}
	return false;
}

#endif

