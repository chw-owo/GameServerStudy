#include "AllocMgr.h"
#include <stdio.h>

TotalAllocInfo g_totalAllocInfo;
vector<AllocInfo> allocInfos;

AllocInfo::AllocInfo(void* ptr, int size, int line, const char* filename)
{
	_ptr = ptr;
	_size = size;
	_line = line;
	strcpy_s(_filename, FILE_NAME, filename);
	g_totalAllocInfo.size += size;
	g_totalAllocInfo.count++;
}

bool AllocInfo::operator==(void* ptr)
{
	return(_ptr == ptr);
}

void PrintAlloc(void)
{
	printf("--------------------------------------\n\n");

	printf("Total Alloc Size : %d\n", g_totalAllocInfo.size);
	printf("Total Alloc Count : %d\n\n", g_totalAllocInfo.count);

	vector<AllocInfo>::iterator it;

	for (it = allocInfos.begin(); it != allocInfos.end(); it++)
	{
		printf("Not Release Memory : [%p] %d Bytes\n", (*it)._ptr, (*it)._size);
		printf("File : %s : %d\n\n", (*it)._filename, (*it)._line);
	}

	printf("--------------------------------------\n");
}