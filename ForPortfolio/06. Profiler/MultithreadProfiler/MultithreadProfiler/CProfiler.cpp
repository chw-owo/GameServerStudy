#include "CProfiler.h"
#include <stdio.h>
//==========================================================================

ProfilerMgr* g_profilerMgr = new ProfilerMgr;
LARGE_INTEGER g_freq;

ProfilerMgr::ProfilerMgr()
{
	_tlsIdx = TlsAlloc();
	QueryPerformanceFrequency(&g_freq);
}

ProfilerMgr::~ProfilerMgr()
{
	for (int i = 0; i < _profilerIdx; i++)
	{
		delete _profilers[i];
	}
}

Profiler* ProfilerMgr::GetProfiler()
{
	Profiler* profiler = (Profiler*)TlsGetValue(_tlsIdx);
	if (profiler == nullptr)
	{
		profiler = new Profiler;
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

void ProfilerMgr::ProfileBegin(string szName)
{
	unordered_map<string, ProfileResult*>::iterator it = _results.find(szName);
	if(it == _results.end())
	{
		ProfileResult* result = new ProfileResult;
		_results.insert(make_pair(szName, result));
	}
	Profiler* profiler = GetProfiler();
	profiler->ProfileBegin(szName);
}

void ProfilerMgr::ProfileEnd(string szName)
{
	Profiler* profiler = GetProfiler();
	profiler->ProfileEnd(szName);
}

void ProfilerMgr::ProfilePrint(const WCHAR* szFileName)
{
	unordered_map<string, ProfileResult*>::iterator it;
	for (it = _results.begin(); it != _results.end(); it++)
	{
		it->second->_dTotalTimes = 0;
		it->second->_iCalls = 0;
	}

	for (int i = 0; i <= _profilerIdx; i++)
	{
		Profiler* profiler = _profilers[i];
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
	for(it = _results.begin(); it != _results.end(); it++)
	{
		memset(buffer, '\0', BUFFER_SIZE);
		sprintf_s(buffer, BUFFER_SIZE,
			"| %s | %.4lfms | %lld |\n",
			it->first,
			(it->second->_dTotalTimes / it->second->_iCalls) * MS_PER_SEC,
			it->second->_iCalls);
		strcat_s(data, OUTPUT_SIZE, buffer);
		idx++;
	}

	strcat_s(data, OUTPUT_SIZE,
		"=====================================\n\n");

	::printf("%s", data);

	FILE* file;
	errno_t ret;
	ret = _wfopen_s(&file, szFileName, L"wb");
	if (ret != 0)
		wprintf(L"Fail to open %s : %d\n", szFileName, ret);
	fwrite(data, strlen(data), 1, file);
	fclose(file);

	printf("Profile Data Out Success!\n");
}

//==========================================================================

void Profiler::ProfileBegin(string szName)
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

void Profiler::ProfileEnd(string szName)
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
