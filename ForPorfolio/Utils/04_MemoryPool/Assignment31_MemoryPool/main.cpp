#include "MemoryPoolT.h"
#include <stdio.h>
int main()
{
	CHUU::CMemoryPoolT<int> MemPoolT(300, false);
	int* pData1 = MemPoolT.Alloc();
	int* pData2 = MemPoolT.Alloc();
	int* pData3 = MemPoolT.Alloc();

	//pData »ç¿ë
	*pData1 = 1;
	*pData2 = 2;
	*pData3 = 3;
	printf("%d, %d, %d\n", *pData1, *pData2, *pData3);

	MemPoolT.Free(pData3);
	MemPoolT.Free(pData2);
	MemPoolT.Free(pData1);
}