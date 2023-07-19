#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#pragma comment(lib, "winmm.lib")

volatile long g_Data = 0;
volatile long g_Connect = 0;
bool g_Shutdown = false; 

unsigned WINAPI AcceptThread(void* arg)
{
	srand(100);//srand((unsigned)time(NULL));
	while (!g_Shutdown)
	{
		InterlockedIncrement(&g_Connect);
		int random = (rand() % 901) + 100;
		Sleep(random);
	}
	return 0;
}

unsigned WINAPI DisconnectThread(void* arg)
{
	srand(200); //srand((unsigned)time(NULL) + 1000);
	while (!g_Shutdown)
	{
		InterlockedDecrement(&g_Connect);
		int random = (rand() % 901) + 100;
		Sleep(random);
	}
	return 0;
}

// SRWLock
SRWLOCK g_srwlock;
unsigned WINAPI UpdateThread(void* arg)
{
	DWORD updateTimer = timeGetTime();

	while (!g_Shutdown)
	{
		AcquireSRWLockExclusive(&g_srwlock);
		g_Data++;
		ReleaseSRWLockExclusive(&g_srwlock);

		AcquireSRWLockShared(&g_srwlock);
		if (g_Data % 1000 == 0)
			printf("Data: %d\n", g_Data);
		ReleaseSRWLockShared(&g_srwlock);
		
		// if(~)와 printf 도중에 값이 바뀌어서 
		// 1000단위가 아닌 값이 출력되는 걸 막기 위해 
		// g_Data++;를 LockExclusive 안에 포함하고
		// 읽기만 하는 영역은 LockShared 안에 포함한다. 

		Sleep(10);
	}
	return 0;
}

#define threadCnt 5
int wmain(int argc, wchar_t* argv[])
{
	InitializeSRWLock(&g_srwlock);
	timeBeginPeriod(1);

	HANDLE arrThread[threadCnt];
	arrThread[0] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, NULL, 0, nullptr);
	arrThread[1] = (HANDLE)_beginthreadex(NULL, 0, DisconnectThread, NULL, 0, nullptr);
	arrThread[2] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, nullptr);
	arrThread[3] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, nullptr);
	arrThread[4] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, nullptr);

	DWORD shutdownTimer = timeGetTime();
	while (timeGetTime() - shutdownTimer < 20000)
	{
		printf("Connect: %d\n", g_Connect);
		Sleep(1000);
	}

	g_Shutdown = true;
	WaitForMultipleObjects(threadCnt, arrThread, true, INFINITE);
	timeEndPeriod(1);
	return 0;
}


/*
// Critical Section
CRITICAL_SECTION cs_Data;
unsigned WINAPI UpdateThread(void* arg)
{
	while (!g_Shutdown)
	{
		EnterCriticalSection(&cs_Data);
		g_Data++;
		if (g_Data % 1000 == 0)
			printf("Data: %d\n", g_Data);
		LeaveCriticalSection(&cs_Data);

		// if(~)와 printf 도중에 값이 바뀌어서 
		// 1000단위가 아닌 값이 출력되는 걸 막기 위해 
		// g_Data++;도 Critical Section 안에 포함한다. 

		Sleep(10);
	}
	return 0;
}

#define threadCnt 5
int wmain(int argc, wchar_t* argv[])
{
	InitializeCriticalSection(&cs_Data);
	timeBeginPeriod(1);

	HANDLE arrThread[threadCnt];
	arrThread[0] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, NULL, 0, nullptr);
	arrThread[1] = (HANDLE)_beginthreadex(NULL, 0, DisconnectThread, NULL, 0, nullptr);
	arrThread[2] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, nullptr);
	arrThread[3] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, nullptr);
	arrThread[4] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, nullptr);

	DWORD shutdownTimer = timeGetTime();
	while (timeGetTime() - shutdownTimer < 20000)
	{
		printf("Connect: %d\n", g_Connect);
		Sleep(1000);
	}

	g_Shutdown = true;
	WaitForMultipleObjects(threadCnt, arrThread, true, INFINITE);
	timeEndPeriod(1);
	DeleteCriticalSection(&cs_Data);

	return 0;
}
*/