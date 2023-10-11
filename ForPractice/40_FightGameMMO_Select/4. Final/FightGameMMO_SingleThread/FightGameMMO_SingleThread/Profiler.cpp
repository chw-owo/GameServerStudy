#include "Profiler.h"
#include <iostream>
#include <float.h>

#define NAME_LEN 64
#define PROFILE_CNT 32
#define MINMAX_CNT 2
#define BUFFER_SIZE 256
#define OUTPUT_SIZE BUFFER_SIZE * PROFILE_CNT
#define NS_PER_SEC 1000
#define MS_PER_SEC 1000000

LARGE_INTEGER freq;
struct _PROFILE_RESULT {
	long			_flag = 0;						// 프로파일의 사용 여부
	WCHAR			_name[NAME_LEN] = { '/0', };	// 프로파일 샘플 이름

	LARGE_INTEGER	_startTime;						// 프로파일 샘플 실행 시간

	double			_totalTime = 0;					// 전체 사용시간 카운터 Time
	double			_min[MINMAX_CNT] = { DBL_MAX, DBL_MAX };	// 최소 사용시간 카운터 Time
	double			_max[MINMAX_CNT] = { 0, 0 };	// 최대 사용시간 카운터 Time

	__int64			_call = 0;						// 누적 호출 횟수

} PROFILE_RESULT[PROFILE_CNT];

void ProfileBegin(const WCHAR* name)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (PROFILE_RESULT[i]._flag != 0 &&
			wcscmp(PROFILE_RESULT[i]._name, name) == 0)
		{
			_PROFILE_RESULT& pf = PROFILE_RESULT[i];
			//pf._flag = 2;
			QueryPerformanceCounter(&pf._startTime);
			return;
		}

		else if (PROFILE_RESULT[i]._flag == 0)
		{
			_PROFILE_RESULT& pf = PROFILE_RESULT[i];
			pf._flag = 1;

			wcscpy_s(pf._name, NAME_LEN, name);
			QueryPerformanceCounter(&pf._startTime);
			QueryPerformanceFrequency(&freq);

			pf._totalTime = 0;

			for (int min = 0; min < MINMAX_CNT; min++)
				pf._min[min] = DBL_MAX;
			for (int max = 0; max < MINMAX_CNT; max++)
				pf._max[max] = 0;

			pf._call = 0;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		::wprintf(L"profiler is full!: %s\n", name);
		return;
	}
}

void ProfileEnd(const WCHAR* name)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (PROFILE_RESULT[i]._flag != 0 &&
			wcscmp(PROFILE_RESULT[i]._name, name) == 0)
		{
			_PROFILE_RESULT& pf = PROFILE_RESULT[i];

			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			double result =
				(endTime.QuadPart - pf._startTime.QuadPart)
				/ (double)freq.QuadPart;

			pf._totalTime += result;

			for (int min = 0; min < MINMAX_CNT; min++)
			{
				if (pf._min[min] > result)
				{
					if (min == MINMAX_CNT - 1)
					{
						pf._min[min] = result;
					}
					else
					{
						int tmpMin = min;
						while (tmpMin < MINMAX_CNT - 1)
						{
							pf._min[tmpMin + 1] = pf._min[tmpMin];
							tmpMin++;
						}
						pf._min[min] = result;
					}
				}
			}

			for (int max = 0; max < MINMAX_CNT; max++)
			{
				if (pf._max[max] < result)
				{
					if (max == MINMAX_CNT - 1)
					{
						pf._max[max] = result;
					}
					else
					{
						int tmpMax = max;
						while (tmpMax < MINMAX_CNT - 1)
						{
							pf._max[tmpMax + 1] = pf._max[tmpMax];
							tmpMax++;
						}
						pf._max[max] = result;
					}
				}
			}
			pf._call++;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		::wprintf(L"Can't find the profiler: %s\n", name);
		return;
	}
}

void ProfilePrintResult()
{
	::wprintf(
		L"----------------------------------------------\n"
		"| Name | Average | Call/s | Total |\n"
		"----------------------------------------------\n");

	int idx = 0;
	while (PROFILE_RESULT[idx]._flag != 0)
	{
		_PROFILE_RESULT& pf = PROFILE_RESULT[idx];
		::wprintf(
			L"| %ls | %.4lfms | %lld | %.2lfms |\n",
			pf._name,
			(pf._totalTime / pf._call) * MS_PER_SEC,
			pf._call,
			(pf._totalTime / pf._call) * MS_PER_SEC * pf._call);
		idx++;
	}

	::wprintf(L"----------------------------------------------\n\n");
}

void ProfileSaveResult(const WCHAR* filename)
{
	char data[OUTPUT_SIZE] =
		"----------------------------------------------\n"
		"| Name | Average | Call/s | Total |\n"
		"----------------------------------------------\n";

	int idx = 0;
	char buffer[BUFFER_SIZE];
	while (PROFILE_RESULT[idx]._flag != 0)
	{
		_PROFILE_RESULT& pf = PROFILE_RESULT[idx];
		memset(buffer, '\0', BUFFER_SIZE);
		sprintf_s(buffer, BUFFER_SIZE,
			"| %ls | %.4lfms | %lld | %.2lfms |\n",
			pf._name,
			(pf._totalTime / pf._call) * MS_PER_SEC,
			pf._call,
			(pf._totalTime / pf._call) * MS_PER_SEC * pf._call);

		strcat_s(data, OUTPUT_SIZE, buffer);
		idx++;
	}

	strcat_s(data, OUTPUT_SIZE,
		"----------------------------------------------\n\n");

	FILE* file;
	errno_t ret;
	ret = _wfopen_s(&file, filename, L"wb");
	if (ret != 0)
		::wprintf(L"Fail to open %s : %d\n", filename, ret);
	fwrite(data, strlen(data), 1, file);
	fclose(file);
}

void ProfileReset(void)
{
	for (int i = 0; i < PROFILE_CNT; i++)
	{
		memset(&PROFILE_RESULT[i], 0, sizeof(_PROFILE_RESULT));
	}
}

