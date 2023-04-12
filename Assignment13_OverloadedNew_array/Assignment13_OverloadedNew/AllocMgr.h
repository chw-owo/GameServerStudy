#pragma once
#include <tchar.h>
#include <vector>
using namespace std;

#define INFO_SIZE 128

#define FILE_NAME 256
#define FILE_SIZE 1024

struct AllocInfo
{
	bool _use = false;
	void* _ptr = nullptr;
	int _size = 0;
	TCHAR _filename[FILE_NAME] = { _T('\0'), };
	int _line = 0;
	bool _array = false;
};

class AllocMgr
{
public:
	~AllocMgr();
	void PushInfo(AllocInfo info);
	bool FindInfo(void* ptr, AllocInfo* info = nullptr);
	void WriteLog(const TCHAR* msg, 
		void* ptr, const AllocInfo& info);
	void MakeLogFile(TCHAR data[FILE_SIZE]);
	
private:
	int _infoCnt = 0;
	AllocInfo _allocInfos[INFO_SIZE];
};
extern AllocMgr g_AllocMgr;


void SetFileName(TCHAR* filename);

