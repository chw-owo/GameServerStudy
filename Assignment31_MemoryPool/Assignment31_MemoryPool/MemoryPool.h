#pragma once
#ifndef  __CHUU_MEMORY_POOL__
#define  __CHUU_MEMORY_POOL__
#include <new.h>

namespace chuu
{
	template <class DATA>
	class CMemoryPool
	{
	private:
		struct st_BLOCK_NODE
		{
			DATA Data;
			DATA* pNext;
		};

	public:
		CMemoryPool(int iBlockNum, bool bPlacementNew = false);
		virtual	~CMemoryPool();

	public:
		DATA*	Alloc(void);
		bool	Free(DATA* pData);
		int		GetCapacityCount(void) { return _iCapacity; }
		int		GetUseCount(void) { return _iUseCount; }

	private:
		bool _bPlacementNew;
		int _iBlockNum;
		int _iCapacity;
		int _iUseCount;
		st_BLOCK_NODE* _pFreeNode;
	};

	template<class DATA>
	CMemoryPool<DATA>::CMemoryPool(int iBlockNum, bool bPlacementNew)
		:_bPlacementNew(bPlacementNew), _iBlockNum(iBlockNum), 
		_iCapacity(iBlockNum), _iUseCount(0), _pFreeNode(nullptr)
	{
		if (_bPlacementNew)
		{
			// Alloc �� Data�� �����ڸ� ȣ���ϹǷ� �̶� ȣ���ϸ� �ȵ�
		}
		else
		{
			// Alloc �� Data�� �����ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� ��

		}
	}

	template<class DATA>
	CMemoryPool<DATA>::~CMemoryPool()
	{
		if (_bPlacementNew)
		{
			// Free �� Data�� �Ҹ��ڸ� ȣ���ϹǷ� �̶��� ȣ���ϸ� �ȵ�
		}
		else
		{
			// Free �� Data�� �Ҹ��ڸ� ȣ������ �����Ƿ� �̶� ȣ���ؾ� ��
		}
	}

	template<class DATA>
	DATA* CMemoryPool<DATA>::Alloc(void)
	{
		if (_bPlacementNew)
		{
			if (_pFreeNode == nullptr)
			{
				// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�. (���� ����)
			}
			else
			{
				// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�.
			}
		}
		else
		{
			if (_pFreeNode == nullptr)
			{
				// ����ִ� ��尡 ���ٸ� ������ �� Data�� �����ڸ� ȣ���Ѵ�. (���� ����)
			}
			else
			{
				// ����ִ� ��尡 �ִٸ� ������ �� Data�� �����ڸ� ȣ������ �ʴ´�.
			}
		}
		return nullptr;
	}

	template<class DATA>
	bool CMemoryPool<DATA>::Free(DATA* pData)
	{
		if (_bPlacementNew)
		{
			// Data�� �Ҹ��ڸ� ȣ���� �� _pFreeNode�� push�Ѵ�.
		}
		else
		{
			// Data�� �Ҹ��ڸ� ȣ������ �ʰ� _pFreeNode�� push�Ѵ�.
		}
		return false;
	}
}
#endif