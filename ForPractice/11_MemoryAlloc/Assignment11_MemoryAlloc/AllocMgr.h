#pragma once
#include <vector>
using namespace std;

#define ALLOC_MGR
#define FILE_NAME 256

#ifdef ALLOC_MGR
#define _MemoryAlloc(type, count)	MemoryAlloc<type>(count, __LINE__, __FILE__)
#else
#define _MemoryAlloc			
#endif

// AllocInfo =============================================

class AllocInfo
{
public:
	AllocInfo(void* ptr, int size, int line, const char* filename);
	bool operator==(void* ptr);

public:
	void* _ptr = nullptr;
	int _size = 0;
	int _line = 0;
	char _filename[FILE_NAME] = { '\0', };
}; 

struct TotalAllocInfo
{
	int size = 0;
	int count = 0;
};

extern TotalAllocInfo g_totalAllocInfo;
extern vector<AllocInfo> allocInfos;

// Memory Alloc Function =============================================

template<typename T>
T* MemoryAlloc(int cnt, int line, const char* filename)
{
	T* ptr = new T[cnt];

	AllocInfo* info = new AllocInfo(ptr, sizeof(T) * cnt, line, filename);
	allocInfos.push_back(*info);

	return ptr;
}

template<typename T>
void MemoryRelease(T ptr)
{
	vector<AllocInfo>::iterator it;

	// 없는 주소를 지우려고 했을 때
	it = find(allocInfos.begin(), allocInfos.end(), ptr);
	if (it == allocInfos.end())
	{
		printf("it's not allocated data!\n");
		return;
	}

	//AllocInfo* info = &(*it);
	allocInfos.erase(it);
	it = allocInfos.begin();
	//delete info;
	delete ptr;
}

void PrintAlloc(void);

