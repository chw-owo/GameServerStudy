#include <iostream>
#include <windows.h>
#include <process.h>
#include "CRingBuffer.h"
#define SIZE 20
#define VALUE 2

char* g_cmp;
CRingBuffer g_buf;
unsigned int WINAPI Enqueue(void* arg);
unsigned int WINAPI Dequeue(void* arg);

int main()
{
	g_cmp = new char[SIZE];
	memset(g_cmp, VALUE, SIZE);

	HANDLE deq = (HANDLE)_beginthreadex(NULL, 0, Dequeue, nullptr, 0, nullptr);
	HANDLE enq = (HANDLE)_beginthreadex(NULL, 0, Enqueue, nullptr, 0, nullptr);

	HANDLE event = CreateEvent(0, false, false, 0);
	if (event == 0) return 0;
	WaitForSingleObject(event, INFINITE);
	delete[] g_cmp;
}

unsigned int __stdcall Enqueue(void* arg)
{
	char* buf = new char[SIZE];
	memset(buf, VALUE, SIZE);
	for (;;)
	{
		g_buf.Enqueue(buf, SIZE);
		Sleep(10);
	}
	delete[] buf;
	return 0;
}

unsigned int __stdcall Dequeue(void* arg)
{
	char* buf = new char[SIZE];
	memset(buf,0, SIZE);
	for (;;)
	{
		g_buf.Dequeue(buf, SIZE);
	}
	delete[] buf;
	return 0;
}
