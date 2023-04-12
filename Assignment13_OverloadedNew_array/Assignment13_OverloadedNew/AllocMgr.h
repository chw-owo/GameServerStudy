#pragma once
using namespace std;

#define INFO_SIZE 128
#define NAME_SIZE 256
#define file_SIZE 1024

struct AllocInfo
{
	bool _use = false;
	void* _ptr = nullptr;
	int _size = 0;
	char _filename[NAME_SIZE] = { '\0', };
	int _line = 0;
	bool _array = false;
};

class AllocMgr
{
public:
	~AllocMgr();
	void PushInfo(AllocInfo info);
	bool FindInfo(void* ptr, AllocInfo* info = nullptr);
	void WriteLog(const char* msg, 
		void* ptr, const AllocInfo& info);
	void MakeLogfile(char data[file_SIZE]);
	
private:
	int _infoCnt = 0;
	AllocInfo _allocInfos[INFO_SIZE];
};
extern AllocMgr g_AllocMgr;


void SetFileName(char* filename);

