#include "CProfiler.h"
#include <stdio.h>
//==========================================================================

CProfilerMgr* g_profilerMgr = new CProfilerMgr;
LARGE_INTEGER g_freq;

CProfilerMgr::CProfilerMgr()
{
	_tlsIdx = TlsAlloc();
	QueryPerformanceFrequency(&g_freq);
}

CProfilerMgr::~CProfilerMgr()
{
	for (int i = 0; i < _profilerIdx; i++)
	{
		delete _profilers[i];
	}
}

CProfiler* CProfilerMgr::GetCProfiler()
{
	CProfiler* profiler = (CProfiler*)TlsGetValue(_tlsIdx);
	if (profiler == nullptr)
	{
		profiler = new CProfiler;
		long idx = InterlockedIncrement(&_profilerIdx);
		_profilers[idx] = profiler;
		bool ret = TlsSetValue(_tlsIdx, (LPVOID)profiler);
		if (ret == 0)
		{
			int err = GetLastError();
			__debugbreak();
		}
	}
	return profiler;
}

void CProfilerMgr::ProfileBegin(string szName)
{
	unordered_map<string, ProfileResult*>::iterator it = _results.find(szName);
	if (it == _results.end())
	{
		ProfileResult* result = new ProfileResult;
		_results.insert(make_pair(szName, result));
	}
	CProfiler* profiler = GetCProfiler();
	profiler->ProfileBegin(szName);
}

void CProfilerMgr::ProfileEnd(string szName)
{
	CProfiler* profiler = GetCProfiler();
	profiler->ProfileEnd(szName);
}

void CProfilerMgr::ProfilePrint(const WCHAR* szFileName)
{
	unordered_map<string, ProfileResult*>::iterator it;
	for (it = _results.begin(); it != _results.end(); it++)
	{
		it->second->_dTotalTimes = 0;
		it->second->_iCalls = 0;
	}

	for (int i = 0; i <= _profilerIdx; i++)
	{
		CProfiler* profiler = _profilers[i];
		for (int j = 0; j < PROFILE_MAX; j++)
		{
			ProfileData* data = &profiler->_datas[j];
			if (data->_flag != 0)
			{
				it = _results.find(data->_szName);
				LONG64 timeRet = InterlockedExchange64(&data->_iTotalTime, 0);
				LONG64 call = InterlockedExchange64(&data->_iCall, 0);
				it->second->_dTotalTimes += (timeRet / (double)g_freq.QuadPart);
				it->second->_iCalls += call;
			}
		}
	}

	// Print ========================================================

	char data[OUTPUT_SIZE] =
		"\n=====================================\n"
		"| Name | Average | Call |\n"
		"=====================================\n";

	int idx = 0;
	char buffer[BUFFER_SIZE];
	for (it = _results.begin(); it != _results.end(); it++)
	{
		memset(buffer, '\0', BUFFER_SIZE);
		sprintf_s(buffer, BUFFER_SIZE,
			"| %s | %.4lfns | %lld |\n",
			it->first,
			(it->second->_dTotalTimes / it->second->_iCalls) * NS_PER_SEC,
			it->second->_iCalls);
		strcat_s(data, OUTPUT_SIZE, buffer);
		idx++;
	}

	strcat_s(data, OUTPUT_SIZE,
		"=====================================\n\n");

	::printf("%s", data);

	/*
	FILE* file;
	errno_t ret;
	ret = _wfopen_s(&file, szFileName, L"wb");
	if (ret != 0)
		wprintf(L"Fail to open %s : %d\n", szFileName, ret);
	fwrite(data, strlen(data), 1, file);
	fclose(file);

	printf("Profile Data Out Success!\n");
	*/
}

//==========================================================================

void CProfiler::ProfileBegin(string szName)
{
	int i = 0;
	for (; i < PROFILE_MAX; i++)
	{
		if (_datas[i]._flag != 0 && _datas[i]._szName == szName)
		{
			ProfileData& data = _datas[i];
			QueryPerformanceCounter(&data._lStartTime);
			return;
		}

		else if (_datas[i]._flag == 0)
		{
			ProfileData& data = _datas[i];
			data._flag = 1;

			data._szName = szName;
			QueryPerformanceCounter(&data._lStartTime);
			QueryPerformanceFrequency(&g_freq);

			data._iTotalTime = 0;
			data._iCall = 0;
			return;
		}
	}

	if (i == PROFILE_MAX)
	{
		::wprintf(L"profiler is full!: %s\n", szName);
		return;
	}
}

void CProfiler::ProfileEnd(string szName)
{
	int i = 0;
	for (; i < PROFILE_MAX; i++)
	{
		if (_datas[i]._flag != 0 && _datas[i]._szName == szName)
		{
			ProfileData& data = _datas[i];
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			__int64 result = endTime.QuadPart - data._lStartTime.QuadPart;
			data._iTotalTime += result;
			data._iCall++;
			return;
		}
	}

	if (i == PROFILE_MAX)
	{
		::wprintf(L"Can't find the profiler: %s\n", szName);
		return;
	}
}

//==========================================================================
