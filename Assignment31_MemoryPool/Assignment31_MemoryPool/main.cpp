#include "MemoryPool.h"

int main()
{
	chuu::CMemoryPool<int> MemPool(300, false);
	int* pData = MemPool.Alloc();

	//pData ���

	MemPool.Free(pData);
}