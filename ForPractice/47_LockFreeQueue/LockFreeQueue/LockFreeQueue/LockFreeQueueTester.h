#pragma once
#include "LockFreeQueue.h"
#include "TestData.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>
#pragma comment(lib, "winmm.lib")

// Test 
// 1. 잘못된 데이터가 반환되는 상황
// 2. 데이터가 있는데 반환이 안되는 상황
// 3. 넣은 데이터의 개수와 다른 상황

class CLockFreeQueueTester
{
private:
#define DATA_TOTAL 20
#define VALUE_DEF 80
	TestData testData[DATA_TOTAL];

public:
	void StartTest(int threadCount)
	{		
		timeBeginPeriod(1);

		for (int i = 0; i < DATA_TOTAL; i++)
		{
			testData[i].SetData(i, VALUE_DEF + i);
			_queue.Enqueue(&testData[i]);
		}

		HANDLE* threads = new HANDLE[threadCount];
		for (int i = 0; i < threadCount; i++)
		{
			threads[i] = (HANDLE)_beginthreadex(NULL, 0, Test, this, 0, nullptr);
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
#define DEQ_MAX 3
	static unsigned WINAPI Test(void* arg) 
	{
		CLockFreeQueueTester* tester = (CLockFreeQueueTester*)arg;
		CLockFreeQueue* testQueue = &tester->_queue;

		while (1)
		{
			TestData* dequeueData[DEQ_MAX] = { nullptr, };
			for(int i = 0; i < DEQ_MAX; i++)
			{
				dequeueData[i] = testQueue->Dequeue();
			}

			Sleep(0);

			for (int i = 0; i < DEQ_MAX; i++)
			{	
				if(dequeueData[i] != nullptr && dequeueData[i]->_value != -1)
				{
					if (dequeueData[i]->_value > VALUE_DEF + DATA_TOTAL ||
						dequeueData[i]->_value < VALUE_DEF)
					{
						::printf("%d, %d\n", dequeueData[i]->_idx, dequeueData[i]->_value);
						__debugbreak();
					}
					testQueue->Enqueue(dequeueData[i]);
				}
			}
		}

		return 0;
	}

	CLockFreeQueue _queue;
};

