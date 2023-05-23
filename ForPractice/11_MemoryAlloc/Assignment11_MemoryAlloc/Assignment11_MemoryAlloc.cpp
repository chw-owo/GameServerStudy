#include <stdio.h>
#include "AllocMgr.h"

int main()
{
	int* p4 = _MemoryAlloc(int, 1);
	int* p400 = _MemoryAlloc(int, 100);
	char* pZ1 = _MemoryAlloc(char, 50);
	char* pZ2 = _MemoryAlloc(char, 150);
	char* pZ3 = _MemoryAlloc(char, 60);
	char* pZ4 = _MemoryAlloc(char, 70);

	MemoryRelease(p4);
	MemoryRelease(p400);
	MemoryRelease(pZ1);
	MemoryRelease(pZ2);
	//MemoryRelease(pZ3);
	//MemoryRelease(pZ3);
	//MemoryRelease(pZ4);

	PrintAlloc();

}