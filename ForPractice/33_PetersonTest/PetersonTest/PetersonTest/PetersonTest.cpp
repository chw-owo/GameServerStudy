#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#pragma comment(lib, "winmm.lib")

int cnt = 0;
bool g_shutdown = false;
int turn = 0;
bool flag[2] = { false, false };

unsigned char debug = 0x00;

unsigned WINAPI Lock0(void* arg)
{
	for (int i = 0; i < 100000; i++)
	{
		flag[0] = true;
		turn = 0;

		__faststorefence();
		while (flag[1] && turn == 0);

		cnt++;
		flag[0] = false;
	}
	return 0;
}

unsigned WINAPI Lock1(void* arg)
{
	for(int i = 0; i < 100000; i++)
	{
		flag[1] = true;
		turn = 1;

		__faststorefence();
		while (flag[0] && turn == 1);

		cnt++;
		flag[1] = false;
	}
	return 0;
}

#define threadCnt 2
int main()
{
	HANDLE arrThread[threadCnt];
	arrThread[0] = (HANDLE)_beginthreadex(NULL, 0, Lock0, NULL, 0, nullptr);
	arrThread[1] = (HANDLE)_beginthreadex(NULL, 0, Lock1, NULL, 0, nullptr);
	WaitForMultipleObjects(threadCnt, arrThread, true, INFINITE);
	printf("%d", cnt);
	return 0;
}
