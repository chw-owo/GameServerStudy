#include "AllocMgr.h"
#include "FileIO.h"
#include <fileapi.h>
#include <tchar.h>
#include <vector>
#include <time.h>
using namespace std;

#define NUM 8
#define OUTPUT_FILE_NAME 32
#define TIME_BUF 128

AllocMgr g_AllocMgr;
AllocMgr::~AllocMgr()
{
	TCHAR buf[FILE_SIZE] = { '\0', };
	TCHAR data[FILE_SIZE] = { '\0', };
	TCHAR msg[] = _T("LEAK");

	int infoIdx = 0;
	int bufIdx = 0;
	int dataIdx = 0;

	for (int i = 0; i < _infoCnt;)
	{
		if (_allocInfos[infoIdx]._use)
		{
			_stprintf_s(buf, _T("%s [%p] [%d] %s : line is %d\n"),
				msg, _allocInfos[infoIdx]._ptr, _allocInfos[infoIdx]._size,
				_allocInfos[infoIdx]._filename, _allocInfos[infoIdx]._line);
			
			bufIdx = 0;
			while (buf[bufIdx] != _T('\n'))
			{
				data[dataIdx] = buf[bufIdx];
				bufIdx++;
				dataIdx++;
			}
			data[dataIdx] = _T('\n');

			dataIdx++;
			i++;
		}	
		infoIdx++;
	}

	MakeLogFile(data);
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

void AllocMgr::WriteLog(const TCHAR* msg, void* ptr, const AllocInfo& info)
{
	TCHAR data[FILE_SIZE] = { '\0', };

	if (msg == _T("NOALLOC") || msg == _T("NOALLOC[]") )
		_stprintf_s(data, _T("%s [%p]\n"), msg, ptr);
	else
		_stprintf_s(data, _T("%s [%p] [%d] %s : line is %d\n"),
			msg, ptr, info._size, info._filename, info._line);

	MakeLogFile(data);
}

void AllocMgr::MakeLogFile(TCHAR data[FILE_SIZE])
{
	time_t timer = time(NULL);
	struct tm date;
	TCHAR buf[TIME_BUF] = { _T('\0'), };
	localtime_s(&date, &timer);

	TCHAR filename[OUTPUT_FILE_NAME] = { _T('\0'), };
	_stprintf_s(filename, _T("Alloc_%04d%02d%02d_%02d%02d%02d_*.txt"),
		date.tm_year + 1900, date.tm_mon + 1,
		date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);

	char chData[FILE_SIZE];
	WideCharToMultiByte(CP_ACP, 0, data, FILE_SIZE,
		chData, FILE_SIZE, NULL, NULL);

	FILE* file;
	SetFileName(filename);
	OpenFile(filename, &file, _T("w"));
	WriteFile(&file, strlen(chData) + 1, chData);
	CloseFile(&file);
}

void SetFileName(TCHAR* filename) 
{
	int nameIdx = 0;
	WIN32_FIND_DATA findData;
	HANDLE hFile;
	hFile = FindFirstFile((LPCWSTR)filename, &findData);
	
	if (hFile != INVALID_HANDLE_VALUE)
		nameIdx++;
		
	while (FindNextFile(hFile, &findData))
		nameIdx++;

	FindClose(hFile);

	TCHAR filenameIdx[NUM] = { _T('\0'), };
	_stprintf_s(filenameIdx, _T("%d.txt"), nameIdx);

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