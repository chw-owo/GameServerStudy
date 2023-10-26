#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>

#define NODE_MIN 16

template <class DATA>
class CObjectPool_MinLock
{
private:
	struct stNODE
	{
		DATA data;
		size_t tail = nullptr;
	};

public:
	template<typename... Types>
	CObjectPool_MinLock(int blockNum, bool placementNew, Types... args);
	virtual	~CObjectPool_MinLock();

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
	HANDLE _heapHandle[dfTHREAD_MAX];	// ������ �� ������ alloc ���� �ּ�ȭ
	stNODE* _pFreeNode[dfTHREAD_MAX];	// Free�� ���� ������ ���� ����	
	int _freeNodeCnt[dfTHREAD_MAX];		// �� �����尡 ���� Free Node ����
	SRWLOCK _lock[dfTHREAD_MAX];		// for freeNodeCnt & pFreeNode

};

template<class DATA>
template<typename... Types>
CObjectPool_MinLock<DATA>::CObjectPool_MinLock(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum)
{
	_tlsIdx = TlsAlloc();
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);

		_heapHandle[threadIdx] = HeapCreate(HEAP_NO_SERIALIZE, 8192, 0);
		_pFreeNode[threadIdx] = nullptr;
		_freeNodeCnt[threadIdx] = 0;
		InitializeSRWLock(&_lock[threadIdx]);
	}

	if (_blockNum <= 0)
		return;

	if (_placementNew)
	{
		// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵȴ�
		_pFreeNode[threadIdx] = (stNODE*)HeapAlloc(
			_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
		_pFreeNode[threadIdx]->tail = (size_t)nullptr;

		for (int i = 1; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)HeapAlloc(
				_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			p->tail = (size_t)_pFreeNode[threadIdx];
			_pFreeNode[threadIdx] = p;
		}
	}
	else
	{
		// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� �ȴ�
		_pFreeNode[threadIdx] = (stNODE*)HeapAlloc(
			_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
		_pFreeNode[threadIdx]->tail = (size_t)nullptr;
		new (&(_pFreeNode[threadIdx]->data)) DATA(args...);

		for (int i = 1; i < _blockNum; i++)
		{		
			stNODE* p = (stNODE*)HeapAlloc(
				_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));	
			p->tail = (size_t)_pFreeNode[threadIdx];
			new (&(_pFreeNode[threadIdx]->data)) DATA(args...);
			_pFreeNode[threadIdx] = p;
		}
	}

	return;
}

template<class DATA>
CObjectPool_MinLock<DATA>::~CObjectPool_MinLock()
{
	if (_placementNew)
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ������ �ʴ´�
		for (int i = 1; i <= _threadIdx; i++)
		{
			HeapDestroy(_heapHandle[i]);
		}
	}
	else
	{
		// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���Ѵ�
		for (int i = 1; i <= _threadIdx; i++)
		{
			while (_pFreeNode[i] != nullptr)
			{
				size_t next = _pFreeNode[i]->tail;
				(_pFreeNode[i]->data).~DATA();
				_pFreeNode[i] = (stNODE*)next;
			}
			HeapDestroy(_heapHandle[i]);
		}
	}

	TlsFree(_tlsIdx);
}

template<class DATA>
template<typename... Types>
DATA* CObjectPool_MinLock<DATA>::Alloc(Types... args)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);

		_heapHandle[threadIdx] = HeapCreate(HEAP_NO_SERIALIZE, 8192, 0);
		_pFreeNode[threadIdx] = nullptr;
		_freeNodeCnt[threadIdx] = 0;
		InitializeSRWLock(&_lock[threadIdx]);
	}

	if (_pFreeNode[threadIdx] == nullptr)
	{
		// ��尡 ���ٸ� Ÿ �������� ��带 Ž���Ѵ�. 
		int targetIdx = -1; 
		int nodeCnt = -1;
		for (int i = 1; i < _threadIdx; i++)
		{
			if (threadIdx == i) continue;

			AcquireSRWLockShared(&_lock[i]);
			if (_freeNodeCnt[i] > NODE_MIN && _freeNodeCnt[i] > nodeCnt)
			{
				targetIdx = i;
				nodeCnt = _freeNodeCnt[i];
			}
			ReleaseSRWLockShared(&_lock[i]);
		}

		if (targetIdx == -1)
		{
			// Ÿ �����忡�� ��尡 ���ٸ� ��� ����, Data �����ڸ� ȣ���Ѵ� (���� ����)
			stNODE* pNew = (stNODE*)HeapAlloc(
				_heapHandle[threadIdx], HEAP_NO_SERIALIZE, sizeof(stNODE));
			new (&(pNew->data)) DATA(args...);
			return &(pNew->data);
		}
		else
		{
			// Ÿ �����忡 ��尡 NODE_MIN �̻� �ִٸ� Ÿ ������ ����� ������ �����´�

			if (threadIdx > targetIdx)
			{
				AcquireSRWLockExclusive(&_lock[targetIdx]);
				AcquireSRWLockExclusive(&_lock[threadIdx]);

				nodeCnt = nodeCnt / 2;
				_pFreeNode[threadIdx] = _pFreeNode[targetIdx];
				stNODE* next = (stNODE*)_pFreeNode[threadIdx]->tail;

				for (int i = 0; i < nodeCnt; i++)
					next = (stNODE*)next->tail;
				_pFreeNode[targetIdx] = next;

				ReleaseSRWLockExclusive(&_lock[targetIdx]);
				ReleaseSRWLockExclusive(&_lock[threadIdx]);
			}
			else if(threadIdx < targetIdx)
			{
				AcquireSRWLockExclusive(&_lock[threadIdx]);
				AcquireSRWLockExclusive(&_lock[targetIdx]);

				nodeCnt = nodeCnt / 2;
				_pFreeNode[threadIdx] = _pFreeNode[targetIdx];
				stNODE* next = (stNODE*)_pFreeNode[threadIdx]->tail;

				for (int i = 0; i < nodeCnt; i++)
					next = (stNODE*)next->tail;
				_pFreeNode[targetIdx] = next;

				ReleaseSRWLockExclusive(&_lock[threadIdx]);
				ReleaseSRWLockExclusive(&_lock[targetIdx]);
			}
		}
	}

	if (_placementNew)
	{
		AcquireSRWLockExclusive(&_lock[threadIdx]);

		// ��带 ������ �� Data�� �����ڸ� ȣ���Ѵ�
		stNODE* p = _pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)p->tail;
		new (&(p->data)) DATA(args...);
		_freeNodeCnt[threadIdx]--;
		
		ReleaseSRWLockExclusive(&_lock[threadIdx]);
		return &(p->data);
	}
	else
	{
		// ��带 ������ �� Data�� �����ڸ� ȣ������ �ʴ´�
		AcquireSRWLockExclusive(&_lock[threadIdx]);

		stNODE* p = _pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)p->tail;
		_freeNodeCnt[threadIdx]--;

		ReleaseSRWLockExclusive(&_lock[threadIdx]);
		return &(p->data);
	}

	return nullptr;
}

template<class DATA>
bool CObjectPool_MinLock<DATA>::Free(DATA* pData)
{
	int threadIdx = (int)TlsGetValue(_tlsIdx);
	if (threadIdx == 0)
	{
		threadIdx = InterlockedIncrement(&_threadIdx);
		TlsSetValue(_tlsIdx, (LPVOID)threadIdx);
		
		_heapHandle[threadIdx] = HeapCreate(HEAP_NO_SERIALIZE, 8192, 0);
		_pFreeNode[threadIdx] = nullptr;
		_freeNodeCnt[threadIdx] = 0;
		InitializeSRWLock(&_lock[threadIdx]);
	}

	if (_placementNew)
	{
		// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNode�� push�Ѵ�
		AcquireSRWLockExclusive(&_lock[threadIdx]);

		pData->~DATA();
		((stNODE*)pData)->tail = (size_t)_pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)pData;
		_freeNodeCnt[threadIdx]++;

		ReleaseSRWLockExclusive(&_lock[threadIdx]);
		return true;
	}
	else
	{
		// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNode�� push�Ѵ�
		AcquireSRWLockExclusive(&_lock[threadIdx]);

		((stNODE*)pData)->tail = (size_t)_pFreeNode[threadIdx];
		_pFreeNode[threadIdx] = (stNODE*)pData;
		_freeNodeCnt[threadIdx]++;

		ReleaseSRWLockExclusive(&_lock[threadIdx]);
		return true;
	}
	return false;
}
