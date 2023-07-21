#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <synchapi.h>
#include <list>

#pragma comment(lib, "Synchronization.lib")
#pragma comment(lib, "winmm.lib")

using namespace std;
list<int> g_list;
bool g_Shutdown = false;
SRWLOCK g_srwlock;

bool g_printFlag = false;
bool g_deleteFlag = false;
bool g_workerFlag = false;
bool g_saveFlag = false;

unsigned WINAPI PrintThread(void* arg)
{
	bool compare = false;
	list<int>::iterator it;
	while (!g_Shutdown)
	{
		AcquireSRWLockShared(&g_srwlock);
		it = g_list.begin();
		if(it == g_list.end())
		{
			printf("list is empty");
		}
		else
		{
			printf("%d", *it);
			it++;
			for (; it != g_list.end(); it++)
			{
				printf("-%d", *it);
			}
		}
		ReleaseSRWLockShared(&g_srwlock);
		printf("\n");

		WaitOnAddress(&g_printFlag, &compare, sizeof(bool), INFINITE);
	}
	return 0;
}

unsigned WINAPI DeleteThread(void* arg)
{
	bool compare = false;
	while (!g_Shutdown)
	{
		//printf("DeleteThread\n");
		AcquireSRWLockExclusive(&g_srwlock);
		if(!g_list.empty())
			g_list.pop_back();
		ReleaseSRWLockExclusive(&g_srwlock);

		WaitOnAddress(&g_deleteFlag, &compare, sizeof(bool), INFINITE);
	}
	return 0;
}

unsigned WINAPI WorkerThread(void* arg)
{
	bool compare = false;
	srand(GetCurrentThreadId());

	while (!g_Shutdown)
	{
		//printf("WorkerThread(%d)\n", GetCurrentThreadId());
		AcquireSRWLockExclusive(&g_srwlock);
		g_list.push_back(rand());
		ReleaseSRWLockExclusive(&g_srwlock);
		
		WaitOnAddress(&g_workerFlag, &compare, sizeof(bool), INFINITE);
	}
	return 0;
}

#define OUTPUT_SIZE 1024
#define BUFFER_SIZE 8
unsigned WINAPI SaveThread(void* arg)
{
	bool compare = false;
	list<int>::iterator it;
	WaitOnAddress(&g_saveFlag, &compare, sizeof(bool), INFINITE);

	while (!g_Shutdown)
	{
		FILE* file;
		errno_t ret = _wfopen_s(&file, L"output.txt", L"w");
		if(ret != 0)
			wprintf(L"Fail to open %s : %d\n", L"output", ret);
		
		char data[OUTPUT_SIZE] = { '\0' };
		char buffer[BUFFER_SIZE] = { '\0' };

		AcquireSRWLockShared(&g_srwlock);
		it = g_list.begin();
		if (it == g_list.end())
		{
			strcat_s(data, OUTPUT_SIZE, "list is empty");
		}
		else
		{
			sprintf_s(buffer, BUFFER_SIZE, "%d", *it);
			strcat_s(data, OUTPUT_SIZE, buffer);
			it++;
			for (; it != g_list.end(); it++)
			{
				memset(buffer, '\0', BUFFER_SIZE);
				sprintf_s(buffer, BUFFER_SIZE, "-%d", *it);
				strcat_s(data, OUTPUT_SIZE, buffer);
			}
		}
		ReleaseSRWLockShared(&g_srwlock);

		fwrite(data, strlen(data), 1, file);
		fclose(file);
		wprintf(L"Print Data Out Success\n");

		WaitOnAddress(&g_saveFlag, &compare, sizeof(bool), INFINITE);
	}
	return 0;
}

#define threadCnt 6
int wmain(int argc, wchar_t* argv[])
{
	InitializeSRWLock(&g_srwlock);
	timeBeginPeriod(1);

	HANDLE arrThread[threadCnt];
	arrThread[0] = (HANDLE)_beginthreadex(NULL, 0, PrintThread, NULL, 0, nullptr);
	arrThread[1] = (HANDLE)_beginthreadex(NULL, 0, DeleteThread, NULL, 0, nullptr);
	arrThread[2] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, 0, nullptr);
	arrThread[3] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, 0, nullptr);
	arrThread[4] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, 0, nullptr);
	arrThread[5] = (HANDLE)_beginthreadex(NULL, 0, SaveThread, NULL, 0, nullptr);
	
	DWORD printTimer = timeGetTime();
	DWORD deleteTimer = timeGetTime();
	DWORD workerTimer = timeGetTime();
	DWORD saveTimer = timeGetTime();

	while (1)
	{
		// Key 1 - Awake SaveThread
		if (GetAsyncKeyState(0x31) && 
			timeGetTime() - saveTimer >= 300) // 너무 우다다다 눌려서 넣은 부분...
		{
			WakeByAddressSingle(&g_saveFlag);
			saveTimer += 300;
		}

		// Key 2 - Terminate All Threads
		else if (GetAsyncKeyState(0x32)) 
		{
			g_Shutdown = true;
			WakeByAddressSingle(&g_printFlag);
			WakeByAddressSingle(&g_deleteFlag);
			WakeByAddressAll(&g_workerFlag);
			WakeByAddressSingle(&g_saveFlag);
			break;
		}

		if (timeGetTime() - printTimer >= 1000)
		{
			WakeByAddressSingle(&g_printFlag);
			printTimer += 1000;
		}

		if (timeGetTime() - deleteTimer >= 333)
		{
			WakeByAddressSingle(&g_deleteFlag);
			deleteTimer += 333;
		}

		if (timeGetTime() - workerTimer >= 1000)
		{
			WakeByAddressAll(&g_workerFlag);
			workerTimer += 1000;
		}
	}
	
	WaitForMultipleObjects(threadCnt, arrThread, true, INFINITE);
	timeEndPeriod(1);
	printf("\nAll Thread Terminate!\n");
	return 0;
}
