#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>
#include <vector>
#include <algorithm>
using namespace std;

#include "CProfiler.h"

#define NODE_MIN 16

template <class DATA>
class CTlsObjectPool
{
private:
	struct stNODE
	{
		DATA data;
		size_t tail = nullptr;
	};

public:
	template<typename... Types>
	CTlsObjectPool(int blockNum, bool placementNew, Types... args);
	virtual	~CTlsObjectPool();

public:
	template<typename... Types>
	inline DATA* Alloc(Types... args);
	inline bool Free(DATA* pData);

private:
	bool _placementNew;
	int _blockNum;

private:
#define dfTHREAD_MAX 20
	DWORD _tlsIdx = 0;
	volatile long _threadIdx = 0;

private:
	struct FreeNodeCnt
	{
		FreeNodeCnt(int nodeCnt, long idx) : _nodeCnt(nodeCnt), _idx(idx) {}
		bool operator <(FreeNodeCnt& other) {
			return _nodeCnt < other._nodeCnt;
		}

		int _nodeCnt = 0;
		long _idx = 0;
	};

private:
	vector<HANDLE> _heapHandles;	// 스레드 별 힙으로 alloc 경합 최소화
	vector<stNODE*> _pFreeNodes;	// Free된 노드는 스레드 별로 관리	
	vector<FreeNodeCnt*> _freeNodeCnts;		// 각 스레드가 지닌 Free Node 개수
	vector<SRWLOCK*> _locks;		// for freeNodeCnt & pFreeNode
	SRWLOCK _lock;

};

template<class DATA>
template<typename... Types>
CTlsObjectPool<DATA>::CTlsObjectPool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum)
{
	InitializeSRWLock(&_lock);

	// For Fill Index 0
	FreeNodeCnt* idx0Node = new FreeNodeCnt(-1, -1);
	_heapHandles.push_back(INVALID_HANDLE_VALUE);
	_pFreeNodes.push_back((stNODE*)nullptr);
	_freeNodeCnts.push_back(idx0Node);
	_locks.push_back((SRWLOCK*)nullptr);

	_tlsIdx = TlsAlloc();
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	
	if (threadIdx == 0)
	{
		AcquireSRWLockExclusive(&_lock);

		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);

		FreeNodeCnt* node = new FreeNodeCnt(blockNum, threadIdx);
		SRWLOCK* lock = new SRWLOCK;
		InitializeSRWLock(lock);

		_heapHandles.push_back(HeapCreate(HEAP_NO_SERIALIZE, 8192, 0));
		_pFreeNodes.push_back((stNODE*)nullptr);
		_freeNodeCnts.push_back(node);
		_locks.push_back(lock);

		ReleaseSRWLockExclusive(&_lock);
	}

	if (_blockNum <= 0)
		return;

	AcquireSRWLockExclusive(_locks[threadIdx]);

	if (_placementNew)
	{
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다
		for (int i = 0; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)HeapAlloc(
				_heapHandles[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			p->tail = (size_t)_pFreeNodes[threadIdx];
			_pFreeNodes[threadIdx] = p;
		}
	}
	else
	{
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다
		for (int i = 0; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)HeapAlloc(
				_heapHandles[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			p->tail = (size_t)_pFreeNodes[threadIdx];
			new (&(p->data)) DATA(args...);
			_pFreeNodes[threadIdx] = p;
		}
	}

	ReleaseSRWLockExclusive(_locks[threadIdx]);

	return;
}

template<class DATA>
CTlsObjectPool<DATA>::~CTlsObjectPool()
{
	if (_placementNew)
	{
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하지 않는다
		for (int i = 1; i <= _threadIdx; i++)
		{
			HeapDestroy(_heapHandles[i]);
		}
	}
	else
	{
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출한다
		for (int i = 1; i <= _threadIdx; i++)
		{
			while (_pFreeNodes[i] != nullptr)
			{
				size_t next = _pFreeNodes[i]->tail;
				(_pFreeNodes[i]->data).~DATA();
				_pFreeNodes[i] = (stNODE*)next;
			}
			HeapDestroy(_heapHandles[i]);
		}
	}

	TlsFree(_tlsIdx);
}

template<class DATA>
template<typename... Types>
DATA* CTlsObjectPool<DATA>::Alloc(Types... args)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		AcquireSRWLockExclusive(&_lock);

		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);

		FreeNodeCnt* node = new FreeNodeCnt(0, threadIdx);
		SRWLOCK* lock = new SRWLOCK;
		InitializeSRWLock(lock);

		_heapHandles.push_back(HeapCreate(HEAP_NO_SERIALIZE, 8192, 0));
		_pFreeNodes.push_back((stNODE*)nullptr);
		_freeNodeCnts.push_back(node);
		_locks.push_back(lock);
		
		ReleaseSRWLockExclusive(&_lock);
	}

	AcquireSRWLockExclusive(_locks[threadIdx]);

	if (_pFreeNodes[threadIdx] == nullptr)
	{
		// 노드가 없다면 타 스레드의 노드를 탐색한다. 
		bool flag = false;
		AcquireSRWLockExclusive(&_lock);
		
		int threadMax = _freeNodeCnts.size();

		vector<FreeNodeCnt*> freeNodeCnts;
		freeNodeCnts.resize(threadMax);
		std::copy(_freeNodeCnts.begin(), _freeNodeCnts.end(), freeNodeCnts.begin());
		sort(freeNodeCnts.begin(), freeNodeCnts.end());

		ReleaseSRWLockExclusive(&_lock);

		for (int i = 0; i < threadMax; i++)
		{
			int targetIdx = freeNodeCnts[i]->_idx;
			if (targetIdx == -1) continue;
			if (targetIdx == threadIdx) continue;
			if (TryAcquireSRWLockExclusive(_locks[targetIdx]) == 0) continue;

			if (_freeNodeCnts[targetIdx]->_nodeCnt > NODE_MIN)
			{
				int nodeCnt = _freeNodeCnts[targetIdx]->_nodeCnt;
				int moveCnt = nodeCnt / 2;
				_pFreeNodes[threadIdx] = _pFreeNodes[targetIdx]; 

				stNODE* prev = _pFreeNodes[threadIdx];
				stNODE* next = (stNODE*)prev->tail;
				
				for (int i = 1; i < moveCnt; i++)
				{
					prev = next;
					next = (stNODE*)prev->tail;
				}

				prev->tail = NULL;
				_pFreeNodes[targetIdx] = next;

				_freeNodeCnts[targetIdx]->_nodeCnt = nodeCnt - moveCnt;
				_freeNodeCnts[threadIdx]->_nodeCnt = moveCnt;
				flag = true;
			}
			ReleaseSRWLockExclusive(_locks[targetIdx]);
		}
		
		if (!flag)
		{
			// 타 스레드에도 노드가 없다면 노드 생성, Data 생성자를 호출한다 (최초 생성)
			stNODE* pNew = (stNODE*)HeapAlloc(
				_heapHandles[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			new (&(pNew->data)) DATA(args...);

			ReleaseSRWLockExclusive(_locks[threadIdx]);
			return &(pNew->data);
		}
	}
		
	if (_placementNew)
	{
		// 노드를 가져온 후 Data의 생성자를 호출한다
		stNODE* p = _pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)p->tail;
		new (&(p->data)) DATA(args...);
		_freeNodeCnts[threadIdx]->_nodeCnt--;

		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return &(p->data);
	}
	else
	{
		// 노드를 가져온 후 Data의 생성자를 호출하지 않는다
		stNODE* p = _pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)p->tail;
		_freeNodeCnts[threadIdx]->_nodeCnt--;

		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return &(p->data);
	}

	return nullptr;
}

template<class DATA>
bool CTlsObjectPool<DATA>::Free(DATA* pData)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		AcquireSRWLockExclusive(&_lock);

		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);

		FreeNodeCnt* node = new FreeNodeCnt(0, threadIdx);
		SRWLOCK* lock = new SRWLOCK;
		InitializeSRWLock(lock);

		_heapHandles.push_back(HeapCreate(HEAP_NO_SERIALIZE, 8192, 0));
		_pFreeNodes.push_back((stNODE*)nullptr);
		_freeNodeCnts.push_back(node);
		_locks.push_back(lock);

		ReleaseSRWLockExclusive(&_lock);
	}

	if (_placementNew)
	{
		// Data의 소멸자를 호출한 후 _pFreeNodes에 push한다
		AcquireSRWLockExclusive(_locks[threadIdx]);

		pData->~DATA();
		((stNODE*)pData)->tail = (size_t)_pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)pData;
		_freeNodeCnts[threadIdx]->_nodeCnt++;

		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return true;
	}
	else
	{
		// Data의 소멸자를 호출하지 않고 _pFreeNodes에 push한다
		AcquireSRWLockExclusive(_locks[threadIdx]);

		((stNODE*)pData)->tail = (size_t)_pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)pData;
		_freeNodeCnts[threadIdx]->_nodeCnt++;

		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return true;
	}
	return false;
}
