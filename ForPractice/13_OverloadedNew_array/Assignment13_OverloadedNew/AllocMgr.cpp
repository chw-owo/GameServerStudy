#include "AllocMgr.h"
#include "stdio.h"
#include <windows.h>
#include <fileapi.h>
#include <time.h>
using namespace std;

#define NUM 8
#define OUTPUT_NAME_SIZE 32
#define TIME_BUF 128

AllocMgr g_AllocMgr;
AllocMgr::~AllocMgr()
{
	char buf[file_SIZE] = { '\0', };
	char data[file_SIZE] = { '\0', };
	char msg[] = "LEAK";

	int infoIdx = 0;
	int bufIdx = 0;
	int dataIdx = 0;

	for (int i = 0; i < _infoCnt;)
	{
		if (_allocInfos[infoIdx]._use)
		{
			sprintf_s(buf, "%s [%p] [%d] %s : line is %d\n",
				msg, _allocInfos[infoIdx]._ptr, _allocInfos[infoIdx]._size,
				_allocInfos[infoIdx]._filename, _allocInfos[infoIdx]._line);
			
			bufIdx = 0;
			while (buf[bufIdx] != '\n')
			{
				data[dataIdx] = buf[bufIdx];
				bufIdx++;
				dataIdx++;
			}
			data[dataIdx] = '\n';

			dataIdx++;
			i++;
		}	
		infoIdx++;
	}

	MakeLogfile(data);
}

void AllocMgr::PushInfo(AllocInfo info)
{
	_infoCnt++;
	for (int i = 0; i < _infoCnt; i++)
	{
		if (!_allocInfos[i]._use)
		{
			_allocInfos[i] = info;
			break;
		}
	}
}
bool AllocMgr::FindInfo(void* ptr, AllocInfo* info)
{
	int idx = 0;
	for (int i = 0; i < _infoCnt;)
	{
		if (_allocInfos[idx]._ptr == ptr)
		{
			_allocInfos[idx]._use = false;
			_infoCnt--;
			if(info != nullptr)
				info = &(_allocInfos[idx]);
			return true;
		}	
		
		if (_allocInfos[idx]._use)
			i++;
		idx++;
	}
	return false;
}

void AllocMgr::WriteLog(const char* msg, void* ptr, const AllocInfo& info)
{
	char data[file_SIZE] = { '\0', };

	if (msg == "NOALLOC" || msg == "NOALLOC[]" )
		sprintf_s(data, "%s [%p]\n", msg, ptr);
	else
		sprintf_s(data, "%s [%p] [%d] %s : line is %d\n",
			msg, ptr, info._size, info._filename, info._line);

	MakeLogfile(data);
}

void AllocMgr::MakeLogfile(char data[file_SIZE])
{
	time_t timer = time(NULL);
	struct tm date;
	char buf[TIME_BUF] = { '\0', };
	localtime_s(&date, &timer);

	char filename[OUTPUT_NAME_SIZE] = { '\0', };
	sprintf_s(filename, "Alloc_%04d%02d%02d_%02d%02d%02d_*.txt",
		date.tm_year + 1900, date.tm_mon + 1,
		date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);

	FILE* file;
	SetfileName(filename);

	errno_t ret = fopen_s(&file, filename, "w");
	if (ret != 0)
		printf("Fail to open %s : %d\n", filename, ret);
	fwrite(data, strlen(data) + 1, 1, file);
	fclose(file);
}

void SetfileName(char* filename) 
{
	int nameIdx = 0;
	WIN32_FIND_DATA findData;
	HANDLE hfile;
	hfile = FindFirstFile((LPCWSTR)filename, &findData);
	
	if (hfile != INVALID_HANDLE_VALUE)
		nameIdx++;
		
	while (FindNextFile(hfile, &findData))
		nameIdx++;

	FindClose(hfile);

	char filenameIdx[NUM] = { '\0', };
	sprintf_s(filenameIdx, "%d.txt", nameIdx);

	int idx1 = 0;
	int idx2 = 0;
	while (filename[idx1] != '\0')
	{
		if (filename[idx1] == '*')
		{
			while (filenameIdx[idx2] != '\0')
			{
				filename[idx1] = filenameIdx[idx2];
				idx1++;
				idx2++;
			}
			break;
		}		
		idx1++;
	}
	
}