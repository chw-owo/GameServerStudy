#pragma once
#include "CLockFreePool.h"
#include "CPoolTestData.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#pragma comment(lib, "winmm.lib")

class CLockFreePoolTester
{
public:
	CLockFreePoolTester()
	{
		_pPool = new CLockFreePool<CPoolTestData>(0, true);
	}

public:
	void StartTest(int threadCount)
	{
		timeBeginPeriod(1);

		HANDLE* threads = new HANDLE[threadCount];
		for (int i = 0; i < threadCount; i++)
		{
			threads[i] = (HANDLE)_beginthreadex(NULL, 0, Test, _pPool, 0, nullptr);
			if (threads[i] == NULL)
			{
				::printf("Error! %s(%d)\n", __func__, __LINE__);
				__debugbreak();
			}
		}

		WaitForMultipleObjects(threadCount, threads, true, INFINITE);
		timeEndPeriod(1);
	}

private:
	static unsigned WINAPI Test(void* arg)
	{
		CLockFreePool<CPoolTestData>* pPool = (CLockFreePool<CPoolTestData>*)arg;

		while (1)
		{
			// TO-DO: Test
		}

		return 0;
	}

private:
	CLockFreePool<CPoolTestData>* _pPool;
};

