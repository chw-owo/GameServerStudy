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

long cnt = 0;
int* input = nullptr;
LARGE_INTEGER freq;
HANDLE beginComplete = nullptr;
HANDLE endComplete = nullptr;


SRWLOCK lock;
queue<int*> LockQ;
concurrent_queue<int*> StlQ;
CLockFreeQueue<int*>* LockFreeQ;

double MyEnqTimes[dfTHREAD_CNT] = { 0, };
double MyDeqTimes[dfTHREAD_CNT] = { 0, };
double LockEnqTimes[dfTHREAD_CNT] = { 0, };
double LockDeqTimes[dfTHREAD_CNT] = { 0, };
double StlEnqTimes[dfTHREAD_CNT] = { 0, };
double StlDeqTimes[dfTHREAD_CNT] = { 0, };

int MyEnqCalls[dfTHREAD_CNT] = { 0, };
int MyDeqCalls[dfTHREAD_CNT] = { 0, };
int LockEnqCalls[dfTHREAD_CNT] = { 0, };
int LockDeqCalls[dfTHREAD_CNT] = { 0, };
int StlEnqCalls[dfTHREAD_CNT] = { 0, };
int StlDeqCalls[dfTHREAD_CNT] = { 0, };

double MyEnqAvgs[dfTHREAD_CNT] = { 0, };
double MyDeqAvgs[dfTHREAD_CNT] = { 0, };
double LockEnqAvgs[dfTHREAD_CNT] = { 0, };
double LockDeqAvgs[dfTHREAD_CNT] = { 0, };
double StlEnqAvgs[dfTHREAD_CNT] = { 0, };
double StlDeqAvgs[dfTHREAD_CNT] = { 0, };

double MyEnqAvg = 0;
double MyDeqAvg = 0;
double LockEnqAvg = 0;
double LockDeqAvg = 0;
double StlEnqAvg = 0;
double StlDeqAvg = 0;

unsigned __stdcall LockFreeEnqThread(void* arg)
{
    int idx = (int)arg;
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        LockFreeQ->Enqueue(input);
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyEnqTimes[idx] += interval;
        MyEnqCalls[idx]++;
        ret = InterlockedIncrement(&cnt);
    }

    return 0;
}

unsigned __stdcall LockFreeDeqThread(void* arg)
{
    int idx = (int)arg;
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        LockFreeQ->Dequeue();
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyDeqTimes[idx] += interval;
        MyDeqCalls[idx]++;
        ret = InterlockedIncrement(&cnt);
    }

    return 0;
}

unsigned __stdcall LockEnqThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        LockQ.push(input);
        Sleep(0);
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockEnqTimes[idx] += interval;
        LockEnqCalls[idx]++;
        ret = InterlockedIncrement(&cnt);
    }

    return 0;
}

unsigned __stdcall LockDeqThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        LockQ.pop();
        Sleep(0);
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockDeqTimes[idx] += interval;
        LockDeqCalls[idx]++;
        ret = InterlockedIncrement(&cnt);
    }

    return 0;
}

unsigned __stdcall StlEnqThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        StlQ.push(input);
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlEnqTimes[idx] += interval;
        StlEnqCalls[idx]++;
        ret = InterlockedIncrement(&cnt);
    }

    return 0;
}

unsigned __stdcall StlDeqThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    InterlockedExchange(&cnt, 0);
    WaitForSingleObject(beginComplete, INFINITE);

    long ret = InterlockedIncrement(&cnt);
    while (ret < dfTEST_TOTAL_CNT)
    {
        int* tmp;
        QueryPerformanceCounter(&start);
        StlQ.try_pop(tmp);
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlDeqTimes[idx] += interval;
        StlDeqCalls[idx]++;
        ret = InterlockedIncrement(&cnt);
    }

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

}

void printoutResult()
{
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        MyEnqTimes[i] *= MS_PER_SEC;
        MyDeqTimes[i] *= MS_PER_SEC;
        if (MyEnqCalls[i] == 0) MyEnqAvgs[i] = 0;
        else MyEnqAvgs[i] = MyEnqTimes[i] / MyEnqCalls[i];
        if (MyDeqCalls[i] == 0)  MyDeqAvgs[i] = 0;
        else MyDeqAvgs[i] = MyDeqTimes[i] / MyDeqCalls[i];
        MyEnqAvg += MyEnqAvgs[i];
        MyDeqAvg += MyDeqAvgs[i];
    }

    MyEnqAvg = MyEnqAvg / dfTHREAD_CNT;
    MyDeqAvg = MyDeqAvg / dfTHREAD_CNT;

    ::printf("<My Queue>\n");
    ::printf("| EnQ | Total | Call | Avg |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("|     | %f | %d | %f |\n", MyEnqTimes[i], MyEnqCalls[i], MyEnqAvgs[i]);
    }
    ::printf("\n");
    ::printf("| DeQ | Total | Call | Avg |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("|     | %f | %d | %f |\n", MyDeqTimes[i], MyDeqCalls[i], MyDeqAvgs[i]);
    }
    ::printf("\n");
    ::printf("Enq Avg: %f, Deq Avg: %f\n", MyEnqAvg, MyDeqAvg);
    ::printf("\n\n");

    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        LockEnqTimes[i] *= MS_PER_SEC;
        LockDeqTimes[i] *= MS_PER_SEC;
        if (LockEnqCalls[i] == 0) LockEnqAvgs[i] = 0;
        else LockEnqAvgs[i] = LockEnqTimes[i] / LockEnqCalls[i];
        if (LockDeqCalls[i] == 0)  LockDeqAvgs[i] = 0;
        else LockDeqAvgs[i] = LockDeqTimes[i] / LockDeqCalls[i];
        LockEnqAvg += LockEnqAvgs[i];
        LockDeqAvg += LockDeqAvgs[i];
    }

    LockEnqAvg = LockEnqAvg / dfTHREAD_CNT;
    LockDeqAvg = LockDeqAvg / dfTHREAD_CNT;

    ::printf("<Lock Queue>\n");
    ::printf("| EnQ | Total | Call | Avg |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("|     | %f | %d | %f |\n", LockEnqTimes[i], LockEnqCalls[i], LockEnqAvgs[i]);
    }
    ::printf("\n");
    ::printf("| DeQ | Total | Call | Avg |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("|     | %f | %d | %f |\n", LockDeqTimes[i], LockDeqCalls[i], LockDeqAvgs[i]);
    }
    ::printf("\n");
    ::printf("Enq Avg: %f, Deq Avg: %f\n", LockEnqAvg, LockDeqAvg);
    ::printf("\n\n");


    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        StlEnqTimes[i] *= MS_PER_SEC;
        StlDeqTimes[i] *= MS_PER_SEC;
        if (StlEnqCalls[i] == 0) StlEnqAvgs[i] = 0;
        else StlEnqAvgs[i] = StlEnqTimes[i] / StlEnqCalls[i];
        if (StlDeqCalls[i] == 0)  StlDeqAvgs[i] = 0;
        else StlDeqAvgs[i] = StlDeqTimes[i] / StlDeqCalls[i];
        StlEnqAvg += StlEnqAvgs[i];
        StlDeqAvg += StlDeqAvgs[i];
    }

    StlEnqAvg = StlEnqAvg / dfTHREAD_CNT;
    StlDeqAvg = StlDeqAvg / dfTHREAD_CNT;

    ::printf("<Stl Queue>\n");
    ::printf("| EnQ | Total | Call | Avg |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("|     | %f | %d | %f |\n", StlEnqTimes[i], StlEnqCalls[i], StlEnqAvgs[i]);
    }
    ::printf("\n");
    ::printf("| DeQ | Total | Call | Avg |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("|     | %f | %d | %f |\n", StlDeqTimes[i], StlDeqCalls[i], StlDeqAvgs[i]);
    }
    ::printf("\n");
    ::printf("Enq Avg: %f, Deq Avg: %f\n", StlEnqAvg, StlDeqAvg);
    ::printf("\n\n");
}

void Test()
{
    beginComplete = CreateEvent(NULL, true, false, nullptr);
    if (beginComplete == NULL) printf("Error\n");
    endComplete = CreateEvent(NULL, true, false, nullptr);
    if (endComplete == NULL) printf("Error\n");

    LockFreeQ = new CLockFreeQueue<int*>;
    InitializeSRWLock(&lock);
    QueryPerformanceFrequency(&freq);

    comparePerformance();
    printoutResult();

    // delete LockFreeQ;
}
*/