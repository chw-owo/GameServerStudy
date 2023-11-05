#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>
#include <vector>
#include <algorithm>
using namespace std;
#define NODE_MIN 16

template <class T>
class CTlsObjectPool
{
private:
	struct stNODE
	{
		T data;
		size_t tail = nullptr;
	};

public:
	CTlsObjectPool(int blockNum, bool placementNew);
	virtual	~CTlsObjectPool();

public:
	template<typename... Types>
	inline T* Alloc(Types... args);
	inline bool Free(T* pData);

public:
	template<typename... Types>
	inline int RegisterThread(bool alloc, Types... args);

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
	vector<HANDLE> _heapHandles;			// ������ �� ������ alloc ���� �ּ�ȭ
	vector<stNODE*> _pFreeNodes;			// Free�� ���� ������ ���� ����	
	vector<FreeNodeCnt*> _freeNodeCnts;		// �� �����尡 ���� Free Node ����
	vector<SRWLOCK*> _locks;				// for freeNodeCnt & pFreeNode
	SRWLOCK _lock;
};

template<class T>
CTlsObjectPool<T>::CTlsObjectPool(int blockNum, bool placementNew)
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

	return;
}

template<class T>
CTlsObjectPool<T>::~CTlsObjectPool()
{
	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ������ �ʴ´�
		for (int i = 1; i <= _threadIdx; i++)
		{
			HeapDestroy(_heapHandles[i]);
		}
	}
	else
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���Ѵ�
		for (int i = 1; i <= _threadIdx; i++)
		{
			/*
			while (_pFreeNodes[i] != nullptr)
			{
				size_t next = _pFreeNodes[i]->tail;
				(_pFreeNodes[i]->data).~T();
				_pFreeNodes[i] = (stNODE*)next;
			}
			*/
			HeapDestroy(_heapHandles[i]);
		}
	}

	TlsFree(_tlsIdx);
}

template<class T>
template<typename... Types>
T* CTlsObjectPool<T>::Alloc(Types... args)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		__debugbreak();
		threadIdx = RegisterThread(true, args...);
	}
	AcquireSRWLockExclusive(_locks[threadIdx]);

	if (_pFreeNodes[threadIdx] == nullptr)
	{
		__debugbreak();

		// ��尡 ���ٸ� Ÿ �������� ��带 Ž���Ѵ�. 
		AcquireSRWLockExclusive(&_lock);
		bool flag = false;
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
			// Ÿ �����忡�� ��尡 ���ٸ� ��� ����, Data �����ڸ� ȣ���Ѵ� (���� ����)
			stNODE* pNew = (stNODE*)HeapAlloc(
				_heapHandles[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			new (&(pNew->data)) T(args...);
			ReleaseSRWLockExclusive(_locks[threadIdx]);
			return &(pNew->data);
		}
	}

	if (_placementNew)
	{
		// ��带 ������ �� Data�� �����ڸ� ȣ���Ѵ�
		stNODE* p = _pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)p->tail;
		new (&(p->data)) T(args...);
		_freeNodeCnts[threadIdx]->_nodeCnt--;
		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return &(p->data);
	}
	else
	{
		// ��带 ������ �� Data�� �����ڸ� ȣ������ �ʴ´�
		stNODE* p = _pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)p->tail;
		_freeNodeCnts[threadIdx]->_nodeCnt--;
		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return &(p->data);
	}

	return nullptr;
}

template<class T>
bool CTlsObjectPool<T>::Free(T* pData)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		__debugbreak();
		threadIdx = RegisterThread(false);
	}
	AcquireSRWLockExclusive(_locks[threadIdx]);

	if (_placementNew)
	{
		// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNodes�� push�Ѵ�
		pData->~T();
		((stNODE*)pData)->tail = (size_t)_pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)pData;
		_freeNodeCnts[threadIdx]->_nodeCnt++;
		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return true;
	}
	else
	{
		// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNodes�� push�Ѵ�
		((stNODE*)pData)->tail = (size_t)_pFreeNodes[threadIdx];
		_pFreeNodes[threadIdx] = (stNODE*)pData;
		_freeNodeCnts[threadIdx]->_nodeCnt++;
		ReleaseSRWLockExclusive(_locks[threadIdx]);
		return true;
	}
	return false;
}

template<class T>
template<typename... Types>
inline int CTlsObjectPool<T>::RegisterThread(bool alloc, Types... args)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);

	if (threadIdx == 0)
	{
		AcquireSRWLockExclusive(&_lock);

		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);
		FreeNodeCnt* node = new FreeNodeCnt(_blockNum, threadIdx);
		SRWLOCK* lock = new SRWLOCK;
		InitializeSRWLock(lock);

		_heapHandles.push_back(HeapCreate(HEAP_NO_SERIALIZE, 8192, 0));
		_pFreeNodes.push_back((stNODE*)nullptr);
		_freeNodeCnts.push_back(node);
		_locks.push_back(lock);

		ReleaseSRWLockExclusive(&_lock);
	}

	if (_blockNum <= 0 || !alloc) return threadIdx;

	AcquireSRWLockExclusive(_locks[threadIdx]);
	if (_placementNew)
	{
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�
		for (int i = 0; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)HeapAlloc(_heapHandles[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			p->tail = (size_t)_pFreeNodes[threadIdx];
			_pFreeNodes[threadIdx] = p;
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		for (int i = 0; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)HeapAlloc(_heapHandles[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			p->tail = (size_t)_pFreeNodes[threadIdx];
			new (&(p->data)) T(args...);
			_pFreeNodes[threadIdx] = p;
		}
	}
	ReleaseSRWLockExclusive(_locks[threadIdx]);

	return threadIdx;
}
