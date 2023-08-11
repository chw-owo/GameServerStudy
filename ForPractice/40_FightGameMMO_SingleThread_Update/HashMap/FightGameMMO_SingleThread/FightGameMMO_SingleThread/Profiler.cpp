#include "Profiler.h"
#include <iostream>
#include <float.h>

#define NAME_LEN 64
#define PROFILE_CNT 32
#define MINMAX_CNT 2
#define BUFFER_SIZE 256
#define OUTPUT_SIZE BUFFER_SIZE * PROFILE_CNT
#define NS_PER_SEC 1000

LARGE_INTEGER freq;
struct _PROFILE_RESULT {
	long			lFlag = 0;						// 프로파일의 사용 여부
	WCHAR			szName[NAME_LEN] = { '/0', };	// 프로파일 샘플 이름

	LARGE_INTEGER	lStartTime;						// 프로파일 샘플 실행 시간

	double			dTotalTime = 0;					// 전체 사용시간 카운터 Time
	double			dMin[MINMAX_CNT] = { DBL_MAX, DBL_MAX };	// 최소 사용시간 카운터 Time
	double			dMax[MINMAX_CNT] = { 0, 0 };	// 최대 사용시간 카운터 Time

	__int64			iCall = 0;						// 누적 호출 횟수

} PROFILE_RESULT[PROFILE_CNT];

void ProfileBegin(const WCHAR* szName)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (PROFILE_RESULT[i].lFlag != 0 &&
			wcscmp(PROFILE_RESULT[i].szName, szName) == 0)
		{
			_PROFILE_RESULT& pf = PROFILE_RESULT[i];
			pf.lFlag = 2;
			QueryPerformanceCounter(&pf.lStartTime);
			return;
		}

		else if (PROFILE_RESULT[i].lFlag == 0)
		{
			_PROFILE_RESULT& pf = PROFILE_RESULT[i];
			pf.lFlag = 1;

			wcscpy_s(pf.szName, NAME_LEN, szName);
			QueryPerformanceCounter(&pf.lStartTime);
			QueryPerformanceFrequency(&freq);

			pf.dTotalTime = 0;

			for (int min = 0; min < MINMAX_CNT; min++)
				pf.dMin[min] = DBL_MAX;
			for (int max = 0; max < MINMAX_CNT; max++)
				pf.dMax[max] = 0;

			pf.iCall = 0;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		wprintf(L"profiler is full!: %s\n", szName);
		return;
	}
}

void ProfileEnd(const WCHAR* szName)
{
	int i = 0;
	for (; i < PROFILE_CNT; i++)
	{
		if (PROFILE_RESULT[i].lFlag != 0 &&
			wcscmp(PROFILE_RESULT[i].szName, szName) == 0)
		{
			_PROFILE_RESULT& pf = PROFILE_RESULT[i];

			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			double result =
				(endTime.QuadPart - pf.lStartTime.QuadPart)
				/ (double)freq.QuadPart;

			pf.dTotalTime += result;

			for (int min = 0; min < MINMAX_CNT; min++)
			{
				if (pf.dMin[min] > result)
				{
					if (min == MINMAX_CNT - 1)
					{
						pf.dMin[min] = result;
					}
					else
					{
						int tmpMin = min;
						while (tmpMin < MINMAX_CNT - 1)
						{
							pf.dMin[tmpMin + 1] = pf.dMin[tmpMin];
							tmpMin++;
						}
						pf.dMin[min] = result;
					}
				}
			}

			for (int max = 0; max < MINMAX_CNT; max++)
			{
				if (pf.dMax[max] < result)
				{
					if (max == MINMAX_CNT - 1)
					{
						pf.dMax[max] = result;
					}
					else
					{
						int tmpMax = max;
						while (tmpMax < MINMAX_CNT - 1)
						{
							pf.dMax[tmpMax + 1] = pf.dMax[tmpMax];
							tmpMax++;
						}
						pf.dMax[max] = result;
					}
				}
			}
			pf.iCall++;
			return;
		}
	}

	if (i == PROFILE_CNT)
	{
		wprintf(L"Can't find the profiler: %s\n", szName);
		return;
	}
}

#define NS_TO_MS 1000
void PrintDataOnConsole()
{
	printf(
		"\n----------------------------------------------\n"
		"| Name | Average | Call/s | Total |\n"
		"----------------------------------------------\n");

	int idx = 0;
	while (PROFILE_RESULT[idx].lFlag != 0)
	{
		_PROFILE_RESULT& pf = PROFILE_RESULT[idx];
		printf(
			"| %ls | %.4lfms | %lld | %.2lfms |\n",
			pf.szName,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC * NS_TO_MS,
			pf.iCall,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC * NS_TO_MS * pf.iCall);
		idx++;
	}

	printf("----------------------------------------------\n");

	/*
	int idx = 0;
	while (PROFILE_RESULT[idx].lFlag != 0)
	{
		_PROFILE_RESULT& pf = PROFILE_RESULT[idx];
		printf(
			"| %ls | %.4lfμs | %.4lfμs | %.4lfμs | %lld |\n",
			pf.szName,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC,
			pf.dMin[0] * NS_PER_SEC,
			pf.dMax[0] * NS_PER_SEC,
			pf.iCall);
		idx++;
	}
	*/

	printf("\n=====================================\n\n");
}

void ProfileDataOutText(const WCHAR* szFileName)
{
	/*
	char data[OUTPUT_SIZE] =
		"\n=================================================\n"
		"| Name | Average | Min | Max | Call | Total |\n"
		"=================================================\n";
	*/
	char data[OUTPUT_SIZE] =
		"\n----------------------------------------------\n"
		"| Name | Average | Call/s | Total |\n"
		"----------------------------------------------\n";
	int idx = 0;
	char buffer[BUFFER_SIZE];
	while (PROFILE_RESULT[idx].lFlag != 0)
	{
		_PROFILE_RESULT& pf = PROFILE_RESULT[idx];
		memset(buffer, '\0', BUFFER_SIZE);
		sprintf_s(buffer, BUFFER_SIZE,
			"| %ls | %.4lfms | %lld | %.2lfms |\n",
			pf.szName,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC * NS_TO_MS,
			pf.iCall,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC * NS_TO_MS * pf.iCall);
		
		/*
		_PROFILE_RESULT& pf = PROFILE_RESULT[idx];
		memset(buffer, '\0', BUFFER_SIZE);
		sprintf_s(buffer, BUFFER_SIZE,
			"| %ls | %.4lfμs | %.4lfμs | %.4lfμs | %lld | %.2lfμs |\n",
			pf.szName,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC,
			pf.dMin[0] * NS_PER_SEC,
			pf.dMax[0] * NS_PER_SEC,
			pf.iCall,
			(pf.dTotalTime / pf.iCall) * NS_PER_SEC * pf.iCall);
		*/

		strcat_s(data, OUTPUT_SIZE, buffer);
		idx++;
	}

	strcat_s(data, OUTPUT_SIZE,
		"----------------------------------------------\n\n");

	FILE* file;
	errno_t ret;
	ret = _wfopen_s(&file, szFileName, L"wb");
	if (ret != 0)
		wprintf(L"Fail to open %s : %d\n", szFileName, ret);
	fwrite(data, strlen(data), 1, file);
	fclose(file);

	//printf("Profile Data Out Success!\n");
}

void ProfileReset(void)
{
	for (int i = 0; i < PROFILE_CNT; i++)
	{
		memset(&PROFILE_RESULT[i], 0, sizeof(_PROFILE_RESULT));
	}
	//printf("Profile Reset Success!\n");
}

