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
			// Alloc 시 Data의 생성자를 호출하므로 이때 호출하면 안됨
		}
		else
		{
			// Alloc 시 Data의 생성자를 호출하지 않으므로 이때 호출해야 됨

		}
	}

	template<class DATA>
	CMemoryPool<DATA>::~CMemoryPool()
	{
		if (_bPlacementNew)
		{
			// Free 시 Data의 소멸자를 호출하므로 이때는 호출하면 안됨
		}
		else
		{
			// Free 시 Data의 소멸자를 호출하지 않으므로 이때 호출해야 됨
		}
	}

	template<class DATA>
	DATA* CMemoryPool<DATA>::Alloc(void)
	{
		if (_bPlacementNew)
		{
			if (_pFreeNode == nullptr)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다. (최초 생성)
			}
			else
			{
				// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출한다.
			}
		}
		else
		{
			if (_pFreeNode == nullptr)
			{
				// 비어있는 노드가 없다면 생성한 후 Data의 생성자를 호출한다. (최초 생성)
			}
			else
			{
				// 비어있는 노드가 있다면 가져온 후 Data의 생성자를 호출하지 않는다.
			}
		}
		return nullptr;
	}

	template<class DATA>
	bool CMemoryPool<DATA>::Free(DATA* pData)
	{
		if (_bPlacementNew)
		{
			// Data의 소멸자를 호출한 후 _pFreeNode에 push한다.
		}
		else
		{
			// Data의 소멸자를 호출하지 않고 _pFreeNode에 push한다.
		}
		return false;
	}
}
#endif