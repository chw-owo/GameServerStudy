#include "CrashDump.h"
#include "LockFreeStack.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#pragma comment(lib, "winmm.lib")

CCrashDump dump;
CLockFreeStack<int> g_Stack;
CLockFreePool<int> g_Pool(2000, false);
unsigned WINAPI LockFreeStackTest(void* arg);
unsigned WINAPI LockFreePoolTest(void* arg);

__declspec(thread) int* arr[100000];

int main()
{
	timeBeginPeriod(1);

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCnt = (int)si.dwNumberOfProcessors / 2;
	HANDLE* threads = new HANDLE[threadCnt];

	for (int i = 0; i < threadCnt; i++)
	{
		threads[i] = (HANDLE)_beginthreadex(
			NULL, 0, LockFreeStackTest, nullptr, 0, nullptr);
		if (threads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}

	WaitForMultipleObjects(threadCnt, threads, true, INFINITE);
	::printf("\nAll Thread Terminate!\n");

	timeEndPeriod(1);
	return 0;
}

unsigned WINAPI LockFreeStackTest(void* arg)
{
	while (1)
	{
		for (int i = 0; i < 100000; i++)
			g_Stack.Push(i);

		for (int i = 0; i < 100000; i++)
			g_Stack.Pop();
	}
}

unsigned WINAPI LockFreePoolTest(void* arg)
{
	while (1)
	{
		for (int i = 0; i < 100000; i++)
			arr[i] = g_Pool.Alloc(i);

		for (int i = 0; i < 100000; i++)
			g_Pool.Free(arr[i]);
	}
}
