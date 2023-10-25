#pragma once
#ifndef  __CHUU_MEMORY_POOL_MEMORY__
#define  __CHUU_MEMORY_POOL_MEMORY__
#include <new.h>

//#define __CHUU_MEMORY_POOL_MEMORY_DEBUG__
#ifdef __CHUU_MEMORY_POOL_MEMORY_DEBUG__
#include <stdio.h>
#endif

namespace CHUU
{
	class CMemoryPoolM
	{
	private:
		struct st_BLOCK_NODE
		{

		};

	public:
		CMemoryPoolM(int iBlockSize);
		virtual	~CMemoryPoolM();

	public:
		char*	Alloc(int allocSize);
		bool	Free(char* pData);

	private:
		int _iBlockSize;		
		st_BLOCK_NODE* _pFreeNode;

#ifdef __CHUU_MEMORY_POOL_MEMORY_DEBUG__
	public:
		int		GetCapacity(void) { return _iCapacity; }
		int		GetUseSize(void) { return _iUseSize; }

	private:
		int _iCapacity;
		int _iUseSize;
#endif
	};
}
#endif