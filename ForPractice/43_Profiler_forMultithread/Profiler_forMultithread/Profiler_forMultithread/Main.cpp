#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <time.h>
#include "Profiler.h"

#pragma comment(lib, "Synchronization.lib")
#pragma comment(lib, "winmm.lib")

#ifdef USE_STLS
__declspec (thread) CProfiler* pSTLSProfiler = new CProfiler;
#endif

#define threadCnt 6
bool g_bTerminate = false;

unsigned WINAPI Func1(void* arg);
unsigned WINAPI Func2(void* arg);
unsigned WINAPI Func3(void* arg);

int wmain(int argc, wchar_t* argv[])
{
	timeBeginPeriod(1);

	HANDLE arrThread[threadCnt];
	arrThread[0] = (HANDLE)_beginthreadex(NULL, 0, Func1, NULL, 0, nullptr);
	arrThread[1] = (HANDLE)_beginthreadex(NULL, 0, Func2, NULL, 0, nullptr);
	arrThread[2] = (HANDLE)_beginthreadex(NULL, 0, Func2, NULL, 0, nullptr);
	arrThread[3] = (HANDLE)_beginthreadex(NULL, 0, Func3, NULL, 0, nullptr);
	arrThread[4] = (HANDLE)_beginthreadex(NULL, 0, Func3, NULL, 0, nullptr);
	arrThread[5] = (HANDLE)_beginthreadex(NULL, 0, Func3, NULL, 0, nullptr);

	
	while (!g_bTerminate)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_bTerminate = true;
		}

		if (GetAsyncKeyState(0x31))
		{
			PRO_PRINT();
		}

		if (GetAsyncKeyState(0x32))
		{
			PRO_SAVE(L"result");
		}

		if (GetAsyncKeyState(0x33))
		{
			PRO_PRINT_ADDUP();
		}

		if (GetAsyncKeyState(0x34))
		{
			PRO_SAVE_ADDUP(L"result");
		}
	}

	WaitForMultipleObjects(threadCnt, arrThread, true, INFINITE);
	timeEndPeriod(1);
	::printf("\nAll Thread Terminate!\n");
	return 0;
    
}

unsigned WINAPI Func1(void* arg)
{
	::printf("\nThread %d Start!\n", GetCurrentThreadId());
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());

	int a = 0;

	while(!g_bTerminate)
	{
		PRO_BEGIN(L"Func 1-1");
		a++;
		PRO_END(L"Func 1-1");

		PRO_BEGIN(L"Func 1-2");
		a--;
		PRO_END(L"Func 1-2");
	}

	::printf("\nThread %d Terminate!\n", GetCurrentThreadId());
	return 0;
}

unsigned WINAPI Func2(void* arg)
{
	::printf("\nThread %d Start!\n", GetCurrentThreadId());
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());

	int a = 0;
	int b = 0;

	while (!g_bTerminate)
	{
		PRO_BEGIN(L"Func 2-1");
		a++;
		b++;
		PRO_BEGIN(L"Func 2-2");
		a--;
		b++;
		b--;
		PRO_END(L"Func 2-1");
		b--;
		PRO_END(L"Func 2-2");
	}

	::printf("\nThread %d Terminate!\n", GetCurrentThreadId());
	return 0;
}

unsigned WINAPI Func3(void* arg)
{
	::printf("\nThread %d Start!\n", GetCurrentThreadId());
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());

	int a = 0;
	int b = 0;
	int c = 5;

	while (!g_bTerminate)
	{
		PRO_BEGIN(L"Func 3-1");
		a++;
		b++;
		PRO_BEGIN(L"Func 3-2");
		a *= c;
		a--;
		b++;
		b *= c;
		b--;
		PRO_END(L"Func 3-2");
		b--;
		PRO_END(L"Func 3-1");
	}

	::printf("\nThread %d Terminate!\n", GetCurrentThreadId());
	return 0;
}