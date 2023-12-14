#pragma once
#include <new.h>
#include <stdlib.h>
#include <windows.h>

template <class T>
class CObjectPool
{
private:
	struct stNODE
	{
		T data;
		size_t tail = nullptr;
	};

public:
	template<typename... Types>
	CObjectPool(int blockNum, bool placementNew, Types... args);
	virtual	~CObjectPool();

public:
	template<typename... Types>
	inline T* Alloc(Types... args);
	inline bool Free(T* pData);

private:
	long _poolSize = 0;
	long _nodeCount = 0;

public:
	long GetPoolSize() { return _poolSize; }
	long GetNodeCount() { return _nodeCount; }

private:
	bool _placementNew;
	int _blockNum;
	stNODE* _pFreeNode = nullptr;
};

template<class T>
template<typename... Types>
CObjectPool<T>::CObjectPool(int blockNum, bool placementNew, Types... args)
	:_placementNew(placementNew), _blockNum(blockNum), _pFreeNode(nullptr)
{
	if (_blockNum <= 0)
		return;

	_poolSize = blockNum;
	_nodeCount = 0;

	if (_placementNew)
	{
		// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안된다
		_pFreeNode = (stNODE*)malloc(sizeof(stNODE));
		_pFreeNode->tail = (size_t)nullptr;
		for (int i = 1; i < _blockNum; i++)
		{
			stNODE* p = (stNODE*)malloc(sizeof(stNODE));
			p->tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
	}
	else
	{
		// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 된다
		_pFreeNode = (stNODE*)malloc(sizeof(stNODE));
		_pFreeNode->tail = (size_t)nullptr;
		for (int i = 1; i < _blockNum; i++)
		{
			new (&(_pFreeNode->data)) T(args...);
			stNODE* p = (stNODE*)malloc(sizeof(stNODE));
			p->tail = (size_t)_pFreeNode;
			_pFreeNode = p;
		}
		new (&(_pFreeNode->data)) T(args...);
	}
}

template<class T>
CObjectPool<T>::~CObjectPool()
{
	if (_pFreeNode == nullptr)
		return;

	if (_placementNew)
	{
		// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안된다
		while (_pFreeNode->tail != (size_t)nullptr)
		{
			size_t next = _pFreeNode->tail;
			free(_pFreeNode);
			_pFreeNode = (stNODE*)next;
		}
		free(_pFreeNode);
	}
	else
	{
		// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 된다
		while (_pFreeNode->tail != (size_t)nullptr)
		{
			size_t next = _pFreeNode->tail;
			(_pFreeNode->data).~T();
			free(_pFreeNode);
			_pFreeNode = (stNODE*)next;
		}
		(_pFreeNode->data).~T();
		free(_pFreeNode);
	}
}

template<class T>
template<typename... Types>
T* CObjectPool<T>::Alloc(Types... args)
{
	if (_pFreeNode == nullptr)
	{
		// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다 (최초 생성)
		stNODE* pNew = (stNODE*)malloc(sizeof(stNODE));
		new (&(pNew->data)) T(args...);
		_nodeCount++;
		_poolSize++;
		return &(pNew->data);
	}

	if (_placementNew)
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다
		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;
		new (&(p->data)) T(args...);
		_nodeCount++;
		return &(p->data);
	}
	else
	{
		// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출하지 않는다
		stNODE* p = _pFreeNode;
		_pFreeNode = (stNODE*)_pFreeNode->tail;
		_nodeCount++;
		return &(p->data);
	}

	return nullptr;
}

template<class T>
bool CObjectPool<T>::Free(T* pData)
{
	if (_placementNew)
	{
		// Data의 소멸자를 호출한 후 _pFreeNode에 push한다
		pData->~T();
		((stNODE*)pData)->tail = (size_t)_pFreeNode;
		_pFreeNode = (stNODE*)pData;
		_nodeCount--;
		return true;
	}
	else
	{
		// Data의 소멸자를 호출하지 않고 _pFreeNode에 push한다
		((stNODE*)pData)->tail = (size_t)_pFreeNode;
		_pFreeNode = (stNODE*)pData;
		_nodeCount--;
		return true;
	}

	return false;
}