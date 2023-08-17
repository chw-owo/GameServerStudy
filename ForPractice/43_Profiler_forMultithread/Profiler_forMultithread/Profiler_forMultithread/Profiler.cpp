#include "Profiler.h"

void CProfiler::ProfileBegin(const WCHAR* _szName)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (_profileResults[i]._lFlag != none &&
			wcscmp(_profileResults[i]._szName, _szName) == 0)
		{
			_PROFILE_RESULT& pf = _profileResults[i];
			_profileResults[i]._lFlag = begin_in;
			QueryPerformanceCounter(&pf._lStartTime);
			_profileResults[i]._lFlag = begin_out;
			return;
		}

		else if (_profileResults[i]._lFlag == none)
		{
			_PROFILE_RESULT& pf = _profileResults[i];
			_profileResults[i]._lFlag = begin_in;

			wcscpy_s(pf._szName, NAME_LEN, _szName);
			QueryPerformanceCounter(&pf._lStartTime);
			QueryPerformanceFrequency(&_freq);

			pf._dTotalTime = 0;

			for (int min = 0; min < MINMAX_CNT; min++)
				pf._dMin[min] = DBL_MAX;
			for (int max = 0; max < MINMAX_CNT; max++)
				pf._dMax[max] = 0;

			pf._iCall = 0;
			_profileResults[i]._lFlag = begin_out;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		::wprintf(L"profiler is full!: %s\n", _szName);
		return;
	}
}

void CProfiler::ProfileEnd(const WCHAR* _szName)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (_profileResults[i]._lFlag != none &&
			wcscmp(_profileResults[i]._szName, _szName) == 0)
		{
			_profileResults[i]._lFlag = end_in;

			_PROFILE_RESULT& pf = _profileResults[i];

			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			double result =
				(endTime.QuadPart - pf._lStartTime.QuadPart)
				/ (double)_freq.QuadPart;

			pf._dTotalTime += result;

			for (int min = 0; min < MINMAX_CNT; min++)
			{
				if (pf._dMin[min] > result)
				{
					if (min == MINMAX_CNT - 1)
					{
						pf._dMin[min] = result;
					}
					else
					{
						int tmpMin = min;
						while (tmpMin < MINMAX_CNT - 1)
						{
							pf._dMin[tmpMin + 1] = pf._dMin[tmpMin];
							tmpMin++;
						}
						pf._dMin[min] = result;
					}
				}
			}

			for (int max = 0; max < MINMAX_CNT; max++)
			{
				if (pf._dMax[max] < result)
				{
					if (max == MINMAX_CNT - 1)
					{
						pf._dMax[max] = result;
					}
					else
					{
						int tmpMax = max;
						while (tmpMax < MINMAX_CNT - 1)
						{
							pf._dMax[tmpMax + 1] = pf._dMax[tmpMax];
							tmpMax++;
						}
						pf._dMax[max] = result;
					}
				}
			}
			pf._iCall++;

			_profileResults[i]._lFlag = end_out;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		::wprintf(L"Can't find the profiler: %s\n", _szName);
		return;
	}
}


void CProfiler::ProfileReset(void)
{
	for (int i = 0; i < PROFILE_CNT; i++)
	{
		memset(&_profileResults[i], 0, sizeof(_PROFILE_RESULT));
	}
	::printf("Profile Reset Success!\n");
}

CProfilerManager::CProfilerManager()
{
	_profilers.reserve(THREAD_MAX);
}

CProfilerManager::~CProfilerManager()
{
	while(!_profilers.empty())
	{
		CProfiler* pProfiler = _profilers.back();
		delete pProfiler;
		_profilers.pop_back();
	}
}

CProfilerManager* CProfilerManager::GetInstance()
{
	static CProfilerManager _manager;
	return  &_manager;
}

void CProfilerManager::InitializeProfiler()
{
	CProfiler* pProfiler = new CProfiler;
	_profilers.push_back(pProfiler);

#ifdef USE_STLS
	pSTLSProfiler = pProfiler;
#endif
}

void CProfilerManager::PauseAllProfiler()
{
	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end(); iter++)
	{
		CProfiler* p = *iter;

		int idx = 0;
		switch (p->_profileResults[idx]._lFlag)
		{
		case none:
			break;
		case begin_in:
			break;
		case begin_out:
			break;
		case end_in:
			break;
		case end_out:
			break;
		}
	}
}

void CProfilerManager::RestartAllProfiler()
{
	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end(); iter++)
	{
		CProfiler* p = *iter;

		int idx = 0;
		switch (p->_profileResults[idx]._lFlag)
		{
		case none:
			break;
		case begin_in:
			break;
		case begin_out:
			break;
		case end_in:
			break;
		case end_out:
			break;
		}
	}
}

void CProfilerManager::PrintDataOnConsole()
{
	PauseAllProfiler();

	::printf(
		"\n----------------------------------------------\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"----------------------------------------------\n");

	vector<CProfiler*>::iterator iter = _profilers.begin();
	for(; iter != _profilers.end();iter++)
	{
		CProfiler* p = *iter;

		int idx = 0;
		while (p->_profileResults[idx]._lFlag != none)
		{
			_PROFILE_RESULT& pf = p->_profileResults[idx];

#ifdef USE_MS_UNIT
			::printf(
				"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
				pf._szName,
				(pf._dTotalTime / pf._iCall) * MS_PER_SEC,
				pf._dMin[0] * MS_PER_SEC,
				pf._dMax[0] * MS_PER_SEC,
				pf._iCall,
				pf._dTotalTime * MS_PER_SEC);
#endif

#ifdef USE_NS_UNIT
			::printf(
				"| %ls | %.4lfレs | %.4lfレs | %.4lfレs | %lld | %.2lfレs |\n",
				pf._szName,
				(pf._dTotalTime / pf._iCall) * NS_PER_SEC,
				pf._dMin[0] * NS_PER_SEC,
				pf._dMax[0] * NS_PER_SEC,
				pf._iCall,
				pf._dTotalTime * NS_PER_SEC);
#endif

			::printf("----------------------------------------------\n");
			idx++;
		}
	}

	RestartAllProfiler();
}

void CProfilerManager::ProfileDataOutText(const WCHAR* szFileName)
{
	PauseAllProfiler();

	char data[OUTPUT_SIZE] =
		"\n----------------------------------------------\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"----------------------------------------------\n";

	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end(); iter++)
	{
		CProfiler* p = *iter;

		int idx = 0;
		char buffer[BUFFER_SIZE];
		while (p->_profileResults[idx]._lFlag != none)
		{
			_PROFILE_RESULT& pf = p->_profileResults[idx];
			memset(buffer, '\0', BUFFER_SIZE);

#ifdef USE_MS_UNIT
			sprintf_s(buffer, BUFFER_SIZE,
				"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
				pf._szName,
				(pf._dTotalTime / pf._iCall) * MS_PER_SEC,
				pf._dMin[0] * MS_PER_SEC,
				pf._dMax[0] * MS_PER_SEC,
				pf._iCall,
				pf._dTotalTime * MS_PER_SEC);
#endif

#ifdef USE_NS_UNIT

			sprintf_s(buffer, BUFFER_SIZE,
				"| %ls | %.4lfレs | %.4lfレs | %.4lfレs | %lld | %.2lfレs |\n",
				pf._szName,
				(pf._dTotalTime / pf._iCall) * NS_PER_SEC,
				pf._dMin[0] * NS_PER_SEC,
				pf._dMax[0] * NS_PER_SEC,
				pf._iCall,
				pf._dTotalTime * NS_PER_SEC);
#endif

			strcat_s(data, OUTPUT_SIZE, buffer);
			idx++;
		}

		strcat_s(data, OUTPUT_SIZE,
			"----------------------------------------------\n\n");

		FILE* file;
		errno_t ret;
		ret = _wfopen_s(&file, szFileName, L"wb");
		if (ret != 0)
			::wprintf(L"Fail to open %s : %d\n", szFileName, ret);
		fwrite(data, strlen(data), 1, file);
		fclose(file);

		::printf("Profile Data Out Success!\n");
	}

	RestartAllProfiler();
}
