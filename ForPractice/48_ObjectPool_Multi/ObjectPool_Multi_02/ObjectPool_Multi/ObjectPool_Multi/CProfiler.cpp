#include "CProfiler.h"

CProfilerManager* g_pManager = CProfilerManager::GetInstance();

void CProfiler::ProfileBegin(wstring _szName)
{
	int i = 0;
	for (; i < RESULT_PER_PROFILER; i++)
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
			AcquireSRWLockExclusive(&_profileResults[i]._lock);
			_profileResults[i]._useFlag = 1;
			ReleaseSRWLockExclusive(&_profileResults[i]._lock);

			_PROFILE_RESULT& result = _profileResults[i];
			result._szName = _szName;

			AcquireSRWLockExclusive(&g_pManager->_mapLock);
			unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*>::iterator iter
				= g_pManager->_resultAddupMap.find(result._szName);
			if (iter == g_pManager->_resultAddupMap.end())
			{
				_PROFILE_RESULT_FOR_ADDUP* resultAddup = new _PROFILE_RESULT_FOR_ADDUP;
				g_pManager->_resultAddupMap.insert(make_pair(result._szName, resultAddup));
			}
			ReleaseSRWLockExclusive(&g_pManager->_mapLock);

			result._iCall = 0;
			result._dTotalTime = 0;
			for (int min = 0; min < MINMAX_CNT; min++)
				result._dMin[min] = DBL_MAX;
			for (int max = 0; max < MINMAX_CNT; max++)
				result._dMax[max] = 0;

			QueryPerformanceFrequency(&_freq);
			QueryPerformanceCounter(&result._lStartTime);
			
			return;
		}
	}

	if (i == RESULT_PER_PROFILER)
	{
		::wprintf(L"profiler is full!: %s\n", _szName);
		return;
	}
}

void CProfiler::ProfileEnd(wstring _szName)
{
	int i = 0;
	for (; i < RESULT_PER_PROFILER; i++)
	{
		if (_profileResults[i]._useFlag != 0 &&
			_profileResults[i]._szName == _szName)
		{
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			_PROFILE_RESULT& result = _profileResults[i];
			double interval = (endTime.QuadPart - result._lStartTime.QuadPart)/(double)_freq.QuadPart;

			AcquireSRWLockExclusive(&_profileResults[i]._lock);
			result._iCall++; 
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
			}
			ReleaseSRWLockExclusive(&_profileResults[i]._lock);

			return;
		}
	}

	if (i == RESULT_PER_PROFILER)
	{
		::wprintf(L"Can't find the profiler: %s\n", _szName);
		return;
	}
}

void CProfiler::ProfileReset(void)
{
	for (int i = 0; i < RESULT_PER_PROFILER; i++)
	{
		AcquireSRWLockExclusive(&_profileResults[i]._lock);
		_profileResults[i].Reset();
		ReleaseSRWLockExclusive(&_profileResults[i]._lock);
	}
}

CProfilerManager::CProfilerManager()
{
	InitializeSRWLock(&_profilersLock);
	InitializeSRWLock(&_mapLock);
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
	LPVOID value = TlsGetValue(_tlsIdx);
	if (value == nullptr)
	{
		CProfiler* profile = new CProfiler;
		TlsSetValue(_tlsIdx, (LPVOID)profile);
		AcquireSRWLockExclusive(&_profilersLock);
		_profilers.push_back(profile);
		ReleaseSRWLockExclusive(&_profilersLock);
	}
}

void CProfilerManager::PrintResult(void)
{
	AcquireSRWLockShared(&_profilersLock);
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

		for (int i = 0; i < RESULT_PER_PROFILER; i++)
		{
			AcquireSRWLockShared(&pf->_profileResults[i]._lock);
			if (pf->_profileResults[i]._useFlag != 0)
			{
				_PROFILE_RESULT result = pf->_profileResults[i];
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);

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

			}
			else
			{
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);
			}
		}
	}
	ReleaseSRWLockShared(&_profilersLock);
}

void CProfilerManager::SaveResult(const wchar_t* szFileName)
{
	AcquireSRWLockShared(&_profilersLock);
	vector<CProfiler*>::iterator iter = _profilers.begin();
	for (; iter != _profilers.end(); iter++)
	{
		char data[OUTPUT_SIZE] =
			"\n----------------------------------------------\n"
			"| Name | Average | Min | Max | Call | Total |\n"
			"----------------------------------------------\n";

		CProfiler* pf = *iter;

		char buffer[BUFFER_SIZE];
		for (int i = 0; i < RESULT_PER_PROFILER; i++)
		{
			AcquireSRWLockShared(&pf->_profileResults[i]._lock);
			if (pf->_profileResults[i]._useFlag != 0)
			{
				_PROFILE_RESULT result = pf->_profileResults[i];
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);

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
			}
			else
			{
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);
			}
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
	ReleaseSRWLockShared(&_profilersLock);
}

void CProfilerManager::PrintResultAddup(void)
{
	AcquireSRWLockShared(&_profilersLock);
	vector<CProfiler*>::iterator profilerIter = _profilers.begin();
	unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*>::iterator resultIter;

	for (; profilerIter != _profilers.end(); profilerIter++)
	{
		CProfiler* pf = *profilerIter;

		for(int i = 0; i < RESULT_PER_PROFILER; i++)
		{ 
			AcquireSRWLockShared(&pf->_profileResults[i]._lock);
			if (pf->_profileResults[i]._useFlag != 0)
			{
				_PROFILE_RESULT result = pf->_profileResults[i];
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);

				AcquireSRWLockShared(&_mapLock);
				resultIter = _resultAddupMap.find(result._szName);
				if (resultIter != _resultAddupMap.end())
				{
					_PROFILE_RESULT_FOR_ADDUP* resultAddup = resultIter->second;
					for (int j = 0; j < MINMAX_CNT; j++)
					{
						if (resultAddup->_dMin[j] > result._dMin[j])
							resultAddup->_dMin[j] = result._dMin[j];
						if (resultAddup->_dMax[j] < result._dMax[j])
							resultAddup->_dMax[j] = result._dMax[j];
					}
					resultAddup->_iCall += result._iCall;
					resultAddup->_dTotalTime += result._dTotalTime;
				}
				ReleaseSRWLockShared(&_mapLock);
			}
			else
			{
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);
			}
		}
	}
	ReleaseSRWLockShared(&_profilersLock);

	::printf(
		"\n----------------------------------------------\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"----------------------------------------------\n");

	AcquireSRWLockShared(&_mapLock);
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
	ReleaseSRWLockShared(&_mapLock);
	::printf("\n----------------------------------------------\n");
}

void CProfilerManager::SaveResultAddup(const wchar_t* szFileName)
{
	AcquireSRWLockShared(&_profilersLock);
	vector<CProfiler*>::iterator profilerIter = _profilers.begin();
	unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*>::iterator resultIter;

	for (; profilerIter != _profilers.end(); profilerIter++)
	{
		CProfiler* pf = *profilerIter;

		for (int i = 0; i < RESULT_PER_PROFILER; i++)
		{
			AcquireSRWLockShared(&pf->_profileResults[i]._lock);
			if (pf->_profileResults[i]._useFlag != 0)
			{
				_PROFILE_RESULT result = pf->_profileResults[i];
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);

				AcquireSRWLockShared(&_mapLock);
				resultIter = _resultAddupMap.find(result._szName);
				if (resultIter != _resultAddupMap.end())
				{
					_PROFILE_RESULT_FOR_ADDUP* resultAddup = resultIter->second;
					for (int j = 0; j < MINMAX_CNT; j++)
					{
						if(resultAddup->_dMin[j] > result._dMin[j])
							resultAddup->_dMin[j] = result._dMin[j];
						if (resultAddup->_dMax[j] < result._dMax[j])
							resultAddup->_dMax[j] = result._dMax[j];
					}
					resultAddup->_iCall += result._iCall;
					resultAddup->_dTotalTime += result._dTotalTime;
				}
				ReleaseSRWLockShared(&_mapLock);
			}
			else
			{
				ReleaseSRWLockShared(&pf->_profileResults[i]._lock);
			}
		}
	}
	ReleaseSRWLockShared(&_profilersLock);

	char data[OUTPUT_SIZE] =
		"\n----------------------------------------------\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"----------------------------------------------\n";

	char buffer[BUFFER_SIZE];

	AcquireSRWLockShared(&_mapLock);
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
	ReleaseSRWLockShared(&_mapLock);

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
