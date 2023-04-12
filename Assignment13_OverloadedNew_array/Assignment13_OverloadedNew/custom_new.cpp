#include "custom_new.h"
#include "AllocMgr.h"

#include <Windows.h>
#include <stringapiset.h>
#include <stdio.h>
#include <stdlib.h>

#undef new
void* operator new(size_t size, const char* File, int Line)
{
	void* ptr = malloc(size);
	TCHAR filename[FILE_NAME] = { '\0', };
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
		File, strlen(File), filename, FILE_NAME);
	
	AllocInfo info;
	info._use = true;
	info._ptr = ptr;
	info._size = size;
	_tcscpy_s(info._filename, FILE_NAME, filename);
	info._line = Line;
	info._array = false;
	g_AllocMgr.PushInfo(info);

	printf("new completely!\n");
	return ptr;
}

void* operator new[](size_t size, const char* File, int Line)
{
	void* ptr = malloc(size);
	TCHAR filename[FILE_NAME] = { '\0', };
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
		File, strlen(File), filename, FILE_NAME);

	AllocInfo info;
	info._use = true;
	info._ptr = ptr;
	info._size = size;
	_tcscpy_s(info._filename, FILE_NAME, filename);
	info._line = Line;
	info._array = true;
	g_AllocMgr.PushInfo(info);

	printf("new[] completely!\n");
	return ptr;
}

void operator delete (void* p, char* File, int Line)
{

}
void operator delete[](void *p, char* File, int Line)
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
		g_AllocMgr.WriteLog(_T("ARRAY"), arrP, info);
		free(arrP);
		return;
	}	
	
	g_AllocMgr.WriteLog(_T("NOALLOC"), p, info);
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
		g_AllocMgr.WriteLog(_T("NOARRAY"), notArrP, info);
		free(notArrP);
		return;
	}

	g_AllocMgr.WriteLog(_T("NOALLOC"), p, info);
}


