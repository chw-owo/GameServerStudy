#include "custom_new.h"
#include "AllocMgr.h"

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stringapiset.h>


#undef new
void* operator new(size_t size, const char* filename, int line)
{
	void* ptr = malloc(size);
	
	AllocInfo info;
	info._use = true;
	info._ptr = ptr;
	info._size = size;
	strcpy_s(info._filename, NAME_SIZE, filename);
	info._line = line;
	info._array = false;
	g_AllocMgr.PushInfo(info);

	printf("new completely!\n");
	return ptr;
}

void* operator new[](size_t size, const char* filename, int line)
{
	void* ptr = malloc(size);

	AllocInfo info;
	info._use = true;
	info._ptr = ptr;
	info._size = size;
	strcpy_s(info._filename, NAME_SIZE, filename);
	info._line = line;
	info._array = true;
	g_AllocMgr.PushInfo(info);

	printf("new[] completely!\n");
	return ptr;
}

void operator delete (void* p, char* file, int line)
{

}
void operator delete[](void *p, char* file, int line)
{

}

void operator delete (void* p)
{
	if (g_AllocMgr.FindInfo(p))
	{
		free(p);
		printf("delete completely!\n");
		return;
	}

	AllocInfo info;
	void* arrP = reinterpret_cast<void*>
		(reinterpret_cast<int>(p) - sizeof(void*));

	if (g_AllocMgr.FindInfo(arrP, &info))
	{
		g_AllocMgr.WriteLog("ARRAY", arrP, info);
		free(arrP);
		return;
	}	
	
	g_AllocMgr.WriteLog("NOALLOC", p, info);
}

void operator delete[](void* p)
{
	if (g_AllocMgr.FindInfo(p))
	{
		free(p);
		printf("delete[] completely!\n");
		return;
	}
	
	AllocInfo info;
	void* notArrP = reinterpret_cast<void*>
		(reinterpret_cast<int>(p) + sizeof(void*));

	if (g_AllocMgr.FindInfo(notArrP, &info))
	{
		g_AllocMgr.WriteLog("NOARRAY", notArrP, info);
		free(notArrP);
		return;
	}

	g_AllocMgr.WriteLog("NOALLOC", p, info);
}


