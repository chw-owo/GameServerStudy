#include "CProfiler.h"

CProfilerManager* g_pManager = CProfilerManager::GetInstance();

void CProfiler::ProfileBegin(wstring _szName)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (_profileResults[i]._useFlag != 0 &&
			_profileResults[i]._szName == _szName)
		{
			_PROFILE_RESULT& result = _profileResults[i];
			QueryPerformanceCounter(&result._lStartTime);
			return;
		}

		else if (_profileResults[i]._useFlag == 0)
		{
			_profileResults[i]._useFlag = 1;
			_PROFILE_RESULT& result = _profileResults[i];
			result._szName = _szName;

			unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*>::iterator iter
				= g_pManager->_resultAddupMap.find(result._szName);

			if (iter == g_pManager->_resultAddupMap.end())
			{
				_PROFILE_RESULT_FOR_ADDUP* resultAddup = new _PROFILE_RESULT_FOR_ADDUP;
				g_pManager->_resultAddupMap.insert(make_pair(result._szName, resultAddup));
			}

			QueryPerformanceCounter(&result._lStartTime);
			QueryPerformanceFrequency(&_freq);

			result._dTotalTime = 0;

			for (int min = 0; min < MINMAX_CNT; min++)
				result._dMin[min] = DBL_MAX;
			for (int max = 0; max < MINMAX_CNT; max++)
				result._dMax[max] = 0;

			result._iCall = 0;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		::wprintf(L"profiler is full!: %s\n", _szName);
		return;
	}
}

void CProfiler::ProfileEnd(wstring _szName)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (_profileResults[i]._useFlag != 0 &&
			_profileResults[i]._szName == _szName)
		{
			_PROFILE_RESULT& result = _profileResults[i];

			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			double interval =
				(endTime.QuadPart - result._lStartTime.QuadPart)
				/ (double)_freq.QuadPart;

			AcquireSRWLockExclusive(&_profileResults[i]._lock);

			result._dTotalTime += interval;

			for (int min = 0; min < MINMAX_CNT; min++)
			{
				if (result._dMin[min] > interval)
				{
					if (min == MINMAX_CNT - 1)
					{
						result._dMin[min] = interval;
					}
					else
					{
						int tmpMin = min;
						while (tmpMin < MINMAX_CNT - 1)
						{
							result._dMin[tmpMin + 1] = result._dMin[tmpMin];
							tmpMin++;
						}
						result._dMin[min] = interval;
					}
				}
			}

			for (int max = 0; max < MINMAX_CNT; max++)
			{
				if (result._dMax[max] < interval)
				{
					if (max == MINMAX_CNT - 1)
					{
						result._dMax[max] = interval;
					}
					else
					{
						int tmpMax = max;
						while (tmpMax < MINMAX_CNT - 1)
						{
							result._dMax[tmpMax + 1] = result._dMax[tmpMax];
							tmpMax++;
						}
						result._dMax[max] = interval;
					}
				}
				result._iCall++;
			}
			ReleaseSRWLockExclusive(&_profileResults[i]._lock);

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
		AcquireSRWLockExclusive(&_profileResults[i]._lock);
		memset(&_profileResults[i], 0, sizeof(_PROFILE_RESULT));
		ReleaseSRWLockExclusive(&_profileResults[i]._lock);
	}
	::printf("Profile Reset Success!\n");
}

CProfilerManager::CProfilerManager()
{
	_tlsIdx = TlsAlloc();
}

CProfilerManager::~CProfilerManager()
{
	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end();)
	{
		CProfiler* profiler = *iter;
		iter = _profilers.erase(iter);
		delete profiler;
	}
}

CProfilerManager* CProfilerManager::GetInstance()
{
	static CProfilerManager _manager;
	return  &_manager;
}

void CProfilerManager::ProfileBegin(wstring _szName)
{
	CProfiler* profile = (CProfiler*)TlsGetValue(_tlsIdx);
	profile->ProfileBegin(_szName);
}

void CProfilerManager::ProfileEnd(wstring _szName)
{
	CProfiler* profile = (CProfiler*)TlsGetValue(_tlsIdx);
	profile->ProfileEnd(_szName);
}

void CProfilerManager::ProfileReset(void)
{
	CProfiler* profile = (CProfiler*)TlsGetValue(_tlsIdx);
	profile->ProfileReset();
}

void CProfilerManager::SetProfiler()
{
	CProfiler* profile = new CProfiler;
	TlsSetValue(_tlsIdx, (LPVOID)profile);
	_profilers.push_back(profile);
}

void CProfilerManager::PrintResult(void)
{
	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end(); iter++)
	{
		CProfiler* pf = *iter;

		::printf(
			"\n<Thread ID: %d>\n"
			"----------------------------------------------\n"
			"| Name | Average | Min | Max | Call | Total |\n"
			"----------------------------------------------\n",
			pf->_threadID);

		int idx = 0;
		while (pf->_profileResults[idx]._useFlag != 0)
		{
			AcquireSRWLockShared(&pf->_profileResults[idx]._lock);
			_PROFILE_RESULT& result = pf->_profileResults[idx];
			ReleaseSRWLockShared(&pf->_profileResults[idx]._lock);

#ifdef USE_MS_UNIT
			::printf(
				"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
				result._szName.c_str(),
				(result._dTotalTime / result._iCall) * MS_PER_SEC,
				result._dMin[0] * MS_PER_SEC,
				result._dMax[0] * MS_PER_SEC,
				result._iCall,
				result._dTotalTime * MS_PER_SEC);
#endif

#ifdef USE_NS_UNIT
			::printf(
				"| %ls | %.4lfレs | %.4lfレs | %.4lfレs | %lld | %.2lfレs |\n",
				result._szName.c_str(),
				(result._dTotalTime / result._iCall) * NS_PER_SEC,
				result._dMin[0] * NS_PER_SEC,
				result._dMax[0] * NS_PER_SEC,
				result._iCall,
				result._dTotalTime * NS_PER_SEC);
#endif

			::printf("----------------------------------------------\n");

			idx++;
		}
	}
}

void CProfilerManager::SaveResult(const wchar_t* szFileName)
{
	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end(); iter++)
	{
		char data[OUTPUT_SIZE] =
			"\n----------------------------------------------\n"
			"| Name | Average | Min | Max | Call | Total |\n"
			"----------------------------------------------\n";

		CProfiler* pf = *iter;

		int idx = 0;
		char buffer[BUFFER_SIZE];
		while (pf->_profileResults[idx]._useFlag != 0)
		{
			AcquireSRWLockShared(&pf->_profileResults[idx]._lock);
			_PROFILE_RESULT& result = pf->_profileResults[idx];
			ReleaseSRWLockShared(&pf->_profileResults[idx]._lock);

			memset(buffer, '\0', BUFFER_SIZE);

#ifdef USE_MS_UNIT
			sprintf_s(buffer, BUFFER_SIZE,
				"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
				result._szName.c_str(),
				(result._dTotalTime / result._iCall) * MS_PER_SEC,
				result._dMin[0] * MS_PER_SEC,
				result._dMax[0] * MS_PER_SEC,
				result._iCall,
				result._dTotalTime * MS_PER_SEC);
#endif

#ifdef USE_NS_UNIT

			sprintf_s(buffer, BUFFER_SIZE,
				"| %ls | %.4lfレs | %.4lfレs | %.4lfレs | %lld | %.2lfレs |\n",
				result._szName.c_str(),
				(result._dTotalTime / result._iCall) * NS_PER_SEC,
				result._dMin[0] * NS_PER_SEC,
				result._dMax[0] * NS_PER_SEC,
				result._iCall,
				result._dTotalTime * NS_PER_SEC);
#endif

			strcat_s(data, OUTPUT_SIZE, buffer);
			idx++;
		}

		strcat_s(data, OUTPUT_SIZE,
			"----------------------------------------------\n\n");

		wchar_t fileName[FILE_NAME_LEN] = { '\0', };
		swprintf_s(fileName, FILE_NAME_LEN, L"%s_%d.txt", szFileName, pf->_threadID);

		FILE* file;
		errno_t ret;
		ret = _wfopen_s(&file, fileName, L"wb");
		if (ret != 0)
			::wprintf(L"Fail to open %s : %d\n", fileName, ret);
		::fwrite(data, strlen(data), 1, file);
		::fclose(file);

		::printf("Save Thread %d Result Success!\n", pf->_threadID);

	}
}

void CProfilerManager::PrintResultAddup(void)
{
	vector<CProfiler*>::iterator profilerIter = _profilers.begin();
	unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*>::iterator resultIter;

	for (; profilerIter != _profilers.end(); profilerIter++)
	{
		CProfiler* pf = *profilerIter;

		int idx = 0;
		while (pf->_profileResults[idx]._useFlag != 0)
		{
			AcquireSRWLockShared(&pf->_profileResults[idx]._lock);
			_PROFILE_RESULT& result = pf->_profileResults[idx];
			ReleaseSRWLockShared(&pf->_profileResults[idx]._lock);

			resultIter = _resultAddupMap.find(result._szName);
			if (resultIter != _resultAddupMap.end())
			{
				_PROFILE_RESULT_FOR_ADDUP* resultAddup = resultIter->second;

				for (int i = 0; i < MINMAX_CNT; i++)
				{
					resultAddup->_dMin[i] += result._dMin[i];
					resultAddup->_dMax[i] += result._dMax[i];
				}
				resultAddup->_iCall += result._iCall;
				resultAddup->_dTotalTime += result._dTotalTime;
			}
			idx++;
		}
	}

	::printf(
		"\n----------------------------------------------\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"----------------------------------------------\n");

	resultIter = _resultAddupMap.begin();
	for (; resultIter != _resultAddupMap.end(); resultIter++)
	{
		_PROFILE_RESULT_FOR_ADDUP* resultAddup = resultIter->second;

#ifdef USE_MS_UNIT
		::printf(
			"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
			resultIter->first.c_str(),
			(resultAddup->_dTotalTime / resultAddup->_iCall) * MS_PER_SEC,
			resultAddup->_dMin[0] * MS_PER_SEC,
			resultAddup->_dMax[0] * MS_PER_SEC,
			resultAddup->_iCall,
			resultAddup->_dTotalTime * MS_PER_SEC);
#endif

#ifdef USE_NS_UNIT
		::printf(
			"| %ls | %.4lfレs | %.4lfレs | %.4lfレs | %lld | %.2lfレs |\n",
			resultIter->first.c_str(),
			(resultAddup->_dTotalTime / resultAddup->_iCall) * NS_PER_SEC,
			resultAddup->_dMin[0] * NS_PER_SEC,
			resultAddup->_dMax[0] * NS_PER_SEC,
			resultAddup->_iCall,
			resultAddup->_dTotalTime * NS_PER_SEC);
#endif	
	}

	::printf("\n----------------------------------------------\n");
}

void CProfilerManager::SaveResultAddup(const wchar_t* szFileName)
{
	vector<CProfiler*>::iterator profilerIter = _profilers.begin();
	unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*>::iterator resultIter;

	for (; profilerIter != _profilers.end(); profilerIter++)
	{
		CProfiler* pf = *profilerIter;

		int idx = 0;
		while (pf->_profileResults[idx]._useFlag != 0)
		{
			AcquireSRWLockShared(&pf->_profileResults[idx]._lock);
			_PROFILE_RESULT result = pf->_profileResults[idx];
			ReleaseSRWLockShared(&pf->_profileResults[idx]._lock);

			resultIter = _resultAddupMap.find(result._szName);
			_PROFILE_RESULT_FOR_ADDUP* resultAddup = resultIter->second;

			for (int i = 0; i < MINMAX_CNT; i++)
			{
				resultAddup->_dMin[i] += result._dMin[i];
				resultAddup->_dMax[i] += result._dMax[i];
			}
			resultAddup->_iCall += result._iCall;
			resultAddup->_dTotalTime += result._dTotalTime;

			idx++;
		}
	}

	char data[OUTPUT_SIZE] =
		"\n----------------------------------------------\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"----------------------------------------------\n";

	char buffer[BUFFER_SIZE];

	resultIter = _resultAddupMap.begin();
	for (; resultIter != _resultAddupMap.end(); resultIter++)
	{
		_PROFILE_RESULT_FOR_ADDUP* resultAddup = resultIter->second;
		memset(buffer, '\0', BUFFER_SIZE);

#ifdef USE_MS_UNIT
		sprintf_s(buffer, BUFFER_SIZE,
			"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
			resultIter->first.c_str(),
			(resultAddup->_dTotalTime / resultAddup->_iCall) * MS_PER_SEC,
			resultAddup->_dMin[0] * MS_PER_SEC,
			resultAddup->_dMax[0] * MS_PER_SEC,
			resultAddup->_iCall,
			resultAddup->_dTotalTime * MS_PER_SEC);
#endif

#ifdef USE_NS_UNIT

		sprintf_s(buffer, BUFFER_SIZE,
			"| %ls | %.4lfms | %.4lfms | %.4lfms | %lld | %.2lfms |\n",
			resultAddup->_szName.c_str(),
			(resultAddup->_dTotalTime / resultAddup->_iCall) * NS_PER_SEC,
			resultAddup->_dMin[0] * NS_PER_SEC,
			resultAddup->_dMax[0] * NS_PER_SEC,
			resultAddup->_iCall,
			resultAddup->_dTotalTime * NS_PER_SEC);
#endif

		strcat_s(data, OUTPUT_SIZE, buffer);
	}

	strcat_s(data, OUTPUT_SIZE,
		"----------------------------------------------\n\n");

	wchar_t fileName[FILE_NAME_LEN] = { '\0', };
	swprintf_s(fileName, FILE_NAME_LEN, L"%s.txt", szFileName);

	FILE* file;
	errno_t ret;
	ret = _wfopen_s(&file, fileName, L"wb");
	if (ret != 0)
		::wprintf(L"Fail to open %s : %d\n", fileName, ret);

	::fwrite(data, strlen(data), 1, file);
	::fclose(file);

	::printf("Save Addup Result Success!\n");
}
