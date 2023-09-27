#include "LockFreeQueue.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#pragma comment(lib, "winmm.lib")

CLockFreeQueue<int> g_Queue;
unsigned WINAPI LockFreeQueueTest(void* arg);

int main()
{
	timeBeginPeriod(1);
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int threadCnt = (int)si.dwNumberOfProcessors - 2;
	HANDLE* threads = new HANDLE[threadCnt];

	for (int i = 0; i < threadCnt; i++)
	{
		threads[i] = (HANDLE)_beginthreadex(
			NULL, 0, LockFreeQueueTest, nullptr, 0, nullptr);
		if (threads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}

	::printf("All Thread Initialize!\n");
	WaitForMultipleObjects(threadCnt, threads, true, INFINITE);
	::printf("All Thread Terminate!\n");

	timeEndPeriod(1);
	return 0;
}

unsigned WINAPI LockFreeQueueTest(void* arg)
{
	while (1)
	{
		for (int i = 0; i < 3; i++)
			g_Queue.Enqueue(i);

		for (int i = 0; i < 3; i++)
			g_Queue.Dequeue();
	}
}