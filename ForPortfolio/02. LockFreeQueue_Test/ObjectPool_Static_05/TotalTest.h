#pragma once
/*
#include <Windows.h>
#include <process.h>  
#include <iostream>
#pragma comment(lib, "winmm.lib")

#include "CLockFreeQueue.h"
#include <queue>
#include <concurrent_queue.h>
using namespace std;
using namespace Concurrency;

#define dfTEST_CNT 25000
#define dfTHREAD_CNT 4
#define dfTEST_TOTAL_CNT 100000
#define MS_PER_SEC 1000000000
#define McS_PER_SEC 1000000

HANDLE beginComplete = nullptr;
HANDLE endComplete = nullptr;
long cnt = 0;

SRWLOCK lock;
queue<int*> LockQ;
concurrent_queue<int*> StlQ;
CLockFreeQueue<int*>* LockFreeQ;
int* input = nullptr;

LARGE_INTEGER freq;
LARGE_INTEGER starttimes[dfTHREAD_CNT];
LARGE_INTEGER endtimes[dfTHREAD_CNT];

double LockFreeEnQTotal = 0;
double LockFreeDeQTotal = 0;
double LockEnQTotal = 0;
double LockDeQTotal = 0;
double StlEnQTotal = 0;
double StlDeQTotal = 0;

unsigned __stdcall LockFreeEnqThread(void* arg)
{
    int idx = (int)arg;
    starttimes[idx].QuadPart = 0;
    endtimes[idx].QuadPart = 0;
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    QueryPerformanceCounter(&starttimes[idx]);
    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        LockFreeQ->Enqueue(input);
        Sleep(0);
        ret = InterlockedIncrement(&cnt);
    }
    QueryPerformanceCounter(&endtimes[idx]);

    return 0;
}

unsigned __stdcall LockFreeDeqThread(void* arg)
{
    int idx = (int)arg;
    starttimes[idx].QuadPart = 0;
    endtimes[idx].QuadPart = 0;
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    QueryPerformanceCounter(&starttimes[idx]);
    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        LockFreeQ->Dequeue();
        Sleep(0);
        ret = InterlockedIncrement(&cnt);
    }
    QueryPerformanceCounter(&endtimes[idx]);

    return 0;
}

unsigned __stdcall LockEnqThread(void* arg)
{
    int idx = (int)arg;
    starttimes[idx].QuadPart = 0;
    endtimes[idx].QuadPart = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    QueryPerformanceCounter(&starttimes[idx]);
    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        AcquireSRWLockExclusive(&lock);
        LockQ.push(input);
        Sleep(0);
        ReleaseSRWLockExclusive(&lock);
        ret = InterlockedIncrement(&cnt);
    }
    QueryPerformanceCounter(&endtimes[idx]);

    return 0;
}

unsigned __stdcall LockDeqThread(void* arg)
{
    int idx = (int)arg;
    starttimes[idx].QuadPart = 0;
    endtimes[idx].QuadPart = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    QueryPerformanceCounter(&starttimes[idx]);
    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        AcquireSRWLockExclusive(&lock);
        LockQ.pop();
        Sleep(0);
        ReleaseSRWLockExclusive(&lock);
        ret = InterlockedIncrement(&cnt);
    }
    QueryPerformanceCounter(&endtimes[idx]);

    return 0;
}

unsigned __stdcall StlEnqThread(void* arg)
{
    int idx = (int)arg;
    starttimes[idx].QuadPart = 0;
    endtimes[idx].QuadPart = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    QueryPerformanceCounter(&starttimes[idx]);
    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        StlQ.push(input);
        Sleep(0);
        ret = InterlockedIncrement(&cnt);
    }
    QueryPerformanceCounter(&endtimes[idx]);

    return 0;
}

unsigned __stdcall StlDeqThread(void* arg)
{
    int idx = (int)arg;
    starttimes[idx].QuadPart = 0;
    endtimes[idx].QuadPart = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    QueryPerformanceCounter(&starttimes[idx]);
    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        int* tmp;
        StlQ.try_pop(tmp);
        Sleep(0);
        ret = InterlockedIncrement(&cnt);
    }
    QueryPerformanceCounter(&endtimes[idx]);

    return 0;
}

void comparePerformance()
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    // Test My Lock-Free Queue: EnQ =======================================================================

    HANDLE MyEnQThreads[dfTHREAD_CNT];
    ResetEvent(beginComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        MyEnQThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeEnqThread, (void*)i, 0, nullptr);
        if (MyEnQThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, MyEnQThreads, true, INFINITE);

    start.QuadPart = LLONG_MAX;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (starttimes[i].QuadPart < start.QuadPart) start.QuadPart = starttimes[i].QuadPart;
    }

    end.QuadPart = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (endtimes[i].QuadPart > end.QuadPart) end.QuadPart = endtimes[i].QuadPart;
    }
    LockFreeEnQTotal = ((end.QuadPart - start.QuadPart) / (double)freq.QuadPart) * McS_PER_SEC;
    ::printf("LockFreeEnQ: %f\n", LockFreeEnQTotal);

    // Test My Lock-Free Queue: DeQ =======================================================================

    HANDLE MyDeQThreads[dfTHREAD_CNT];
    ResetEvent(beginComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        MyDeQThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeDeqThread, (void*)i, 0, nullptr);
        if (MyDeQThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, MyDeQThreads, true, INFINITE);

    start.QuadPart = LLONG_MAX;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (starttimes[i].QuadPart < start.QuadPart) start.QuadPart = starttimes[i].QuadPart;
    }

    end.QuadPart = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (endtimes[i].QuadPart > end.QuadPart) end.QuadPart = endtimes[i].QuadPart;
    }
    LockFreeDeQTotal = ((end.QuadPart - start.QuadPart) / (double)freq.QuadPart) * McS_PER_SEC;
    ::printf("LockFreeDeQ: %f\n", LockFreeDeQTotal);

    // Test Queue with Lock: EnQ =======================================================================

    HANDLE LockEnQThreads[dfTHREAD_CNT];
    ResetEvent(beginComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        LockEnQThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockEnqThread, (void*)i, 0, nullptr);
        if (LockEnQThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, LockEnQThreads, true, INFINITE);

    start.QuadPart = LLONG_MAX;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (starttimes[i].QuadPart < start.QuadPart) start.QuadPart = starttimes[i].QuadPart;
    }

    end.QuadPart = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (endtimes[i].QuadPart > end.QuadPart) end.QuadPart = endtimes[i].QuadPart;
    }
    LockEnQTotal = ((end.QuadPart - start.QuadPart) / (double)freq.QuadPart) * McS_PER_SEC;
    ::printf("LockEnQ: %f\n", LockEnQTotal);

    // Test Queue with Lock: DeQ =======================================================================

    HANDLE LockDeQThreads[dfTHREAD_CNT];
    ResetEvent(beginComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        LockDeQThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockDeqThread, (void*)i, 0, nullptr);
        if (LockDeQThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, LockDeQThreads, true, INFINITE);

    start.QuadPart = LLONG_MAX;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (starttimes[i].QuadPart < start.QuadPart) start.QuadPart = starttimes[i].QuadPart;
    }

    end.QuadPart = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (endtimes[i].QuadPart > end.QuadPart) end.QuadPart = endtimes[i].QuadPart;
    }
    LockDeQTotal = ((end.QuadPart - start.QuadPart) / (double)freq.QuadPart) * McS_PER_SEC;
    ::printf("LockDeQ: %f\n", LockDeQTotal);

    // Test Stl Queue: EnQ =======================================================================

    HANDLE StlEnQThreads[dfTHREAD_CNT];
    ResetEvent(beginComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        StlEnQThreads[i] = (HANDLE)_beginthreadex(NULL, 0, StlEnqThread, (void*)i, 0, nullptr);
        if (StlEnQThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, StlEnQThreads, true, INFINITE);

    start.QuadPart = LLONG_MAX;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (starttimes[i].QuadPart < start.QuadPart) start.QuadPart = starttimes[i].QuadPart;
    }

    end.QuadPart = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (endtimes[i].QuadPart > end.QuadPart) end.QuadPart = endtimes[i].QuadPart;
    }
    StlEnQTotal = ((end.QuadPart - start.QuadPart) / (double)freq.QuadPart) * McS_PER_SEC;
    ::printf("StlEnQ: %f\n", StlEnQTotal);

    // Test Stl Queue: DeQ =======================================================================

    HANDLE StlDeQThreads[dfTHREAD_CNT];
    ResetEvent(beginComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        StlDeQThreads[i] = (HANDLE)_beginthreadex(NULL, 0, StlDeqThread, (void*)i, 0, nullptr);
        if (StlDeQThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, StlDeQThreads, true, INFINITE);

    start.QuadPart = LLONG_MAX;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (starttimes[i].QuadPart < start.QuadPart) start.QuadPart = starttimes[i].QuadPart;
    }

    end.QuadPart = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (endtimes[i].QuadPart > end.QuadPart) end.QuadPart = endtimes[i].QuadPart;
    }
    StlDeQTotal = ((end.QuadPart - start.QuadPart) / (double)freq.QuadPart) * McS_PER_SEC;
    ::printf("StlDeQ: %f\n", StlDeQTotal);
}

void TotalTest()
{
    beginComplete = CreateEvent(NULL, true, false, nullptr);
    if (beginComplete == NULL) printf("Error\n");
    endComplete = CreateEvent(NULL, true, false, nullptr);
    if (endComplete == NULL) printf("Error\n");

    LockFreeQ = new CLockFreeQueue<int*>;
    InitializeSRWLock(&lock);
    QueryPerformanceFrequency(&freq);

    comparePerformance();
    // delete LockFreeQ;
}
*/