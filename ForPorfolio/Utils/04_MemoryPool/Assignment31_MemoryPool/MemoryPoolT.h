#pragma once
#ifndef  __CHUU_MEMORY_POOL_TYPE__
#define  __CHUU_MEMORY_POOL_TYPE__
#include <new.h>
#include <stdlib.h>
//#define __CHUU_MEMORY_POOL_TYPE_DEBUG__

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
#include <stdio.h>
#endif

namespace CHUU
{
	template <class DATA>
	class CMemoryPoolT
	{
	private:

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__

		struct st_BLOCK_NODE
		{
			DATA Data;
			st_BLOCK_NODE* pNext;
		};
#else
		struct st_BLOCK_NODE
		{
			DATA Data;
			st_BLOCK_NODE* pNext = nullptr;
		};
#endif

	public:
		CMemoryPoolT(int iBlockNum, bool bPlacementNew = false);
		virtual	~CMemoryPoolT();

	public:
		DATA*	Alloc(void);
		bool	Free(DATA* pData);

	private:
		bool _bPlacementNew;
		int _iBlockNum;
		st_BLOCK_NODE* _pFreeNode = nullptr;

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
	public:
		int		GetCapacityCount(void) { return _iCapacity; }
		int		GetUseCount(void) { return _iUseCount; }

	private:
		int _iCapacity;
		int _iUseCount;
#endif

	};

	template<class DATA>
	CMemoryPoolT<DATA>::CMemoryPoolT(int iBlockNum, bool bPlacementNew)
		:_bPlacementNew(bPlacementNew), _iBlockNum(iBlockNum), _pFreeNode(nullptr)
	{
#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
		_iCapacity = _iBlockNum;
		_iUseCount = 0;
#endif

		if (_iBlockNum <= 0)
			return;

		if (_bPlacementNew)
		{
			// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵ�
			_pFreeNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			_pFreeNode->pNext = nullptr;
			for (int i = 1; i < _iBlockNum; i++)
			{
				st_BLOCK_NODE* p = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				p->pNext = _pFreeNode;
				_pFreeNode = p;
			}
		}
		else
		{
			// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� ��
			_pFreeNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			_pFreeNode->pNext = nullptr;
			for (int i = 1; i < _iBlockNum; i++)
			{
				new (&(_pFreeNode->Data)) DATA;
				st_BLOCK_NODE* p = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				p->pNext = _pFreeNode;
				_pFreeNode = p;
			}
			new (&(_pFreeNode->Data)) DATA;
		}
	}

	template<class DATA>
	CMemoryPoolT<DATA>::~CMemoryPoolT()
	{
#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
		if (_iUseCount != 0)
		{
			printf("There is Unfree Data!!\n");
		}
#endif
		if (_bPlacementNew)
		{
			// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵ�
			while (_pFreeNode->pNext != nullptr)
			{
				st_BLOCK_NODE* pNext = _pFreeNode->pNext;
				free(_pFreeNode);
				_pFreeNode = pNext;
			}
			free(_pFreeNode);
		}
		else
		{
			// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� ��
			while (_pFreeNode->pNext != nullptr)
			{
				st_BLOCK_NODE* pNext = _pFreeNode->pNext;
				(_pFreeNode->Data).~DATA();
				free(_pFreeNode);
				_pFreeNode = pNext;
			}
			(_pFreeNode->Data).~DATA();
			free(_pFreeNode);
		}
	}

	template<class DATA>
	DATA* CMemoryPoolT<DATA>::Alloc(void)
	{
		if (_pFreeNode == nullptr)
		{
			// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�. (���� ����)

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
			_iCapacity++;
			_iUseCount++;
#endif

			st_BLOCK_NODE* pNew = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			new (&(pNew->Data)) DATA;
			return &(pNew->Data);
		}

		if (_bPlacementNew)
		{
			// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�.

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
			_iUseCount++;
#endif

			st_BLOCK_NODE* p = _pFreeNode;
			_pFreeNode = _pFreeNode->pNext;
			new (&(p->Data)) DATA;
			return &(p->Data);
		}
		else
		{
			// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ������ �ʴ´�.

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
			_iUseCount++;
#endif
			st_BLOCK_NODE* p = _pFreeNode;
			_pFreeNode = _pFreeNode->pNext;
			return &(p->Data);
		}

		return nullptr;
	}

	template<class DATA>
	bool CMemoryPoolT<DATA>::Free(DATA* pData)
	{
		if (_bPlacementNew)
		{
			// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNode�� push�Ѵ�.

#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
			_iUseCount--;
#else
			pData->~DATA();
			((st_BLOCK_NODE*)pData)->pNext = _pFreeNode;
			_pFreeNode = (st_BLOCK_NODE*)pData;
			return true;
#endif
		}
		else
		{
			// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNode�� push�Ѵ�.
#ifdef __CHUU_MEMORY_POOL_TYPE_DEBUG__
			_iUseCount--;
#else
			((st_BLOCK_NODE*)pData)->pNext = _pFreeNode;
			_pFreeNode = (st_BLOCK_NODE*)pData;
			return true;
#endif
		}
		return false;
	}
}
#endif