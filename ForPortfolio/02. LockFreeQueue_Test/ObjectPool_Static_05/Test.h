#pragma once
#include <Windows.h>
#include <process.h>  
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#pragma comment(lib, "winmm.lib")

#include "matplotlibcpp.h"
namespace plt_Enq = matplotlibcpp;
namespace plt_Deq = matplotlibcpp;

#include "CLockFreeQueue.h"
#include <queue>
#include <concurrent_queue.h>
using namespace std;
using namespace Concurrency;

#define dfTEST_CNT 10000
#define dfTHREAD_CNT 4
#define dfTEST_TOTAL_CNT 40000
#define MS_PER_SEC 1000000000

HANDLE beginThreadComplete = nullptr;

double MyEnqTotals[dfTHREAD_CNT];
double MyDeqTotals[dfTHREAD_CNT];
double LockEnqTotals[dfTHREAD_CNT];
double LockDeqTotals[dfTHREAD_CNT];
double StlEnqTotals[dfTHREAD_CNT];
double StlDeqTotals[dfTHREAD_CNT];

double MyEnqDuration;
double MyDeqDuration;
double LockEnqDuration;
double LockDeqDuration;
double StlEnqDuration;
double StlDeqDuration;

struct argForThread
{
    argForThread(size_t Q, int idx, SRWLOCK* lock = nullptr)
        : _Q(Q), _idx(idx), _lock(lock) {};
    size_t _Q;
    int _idx;
    SRWLOCK* _lock;
};

unsigned __stdcall LockFreeQueueThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CLockFreeQueue<int*>* Q = (CLockFreeQueue<int*>*)input->_Q;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double EnqMax = 0;
    double DeqMax = 0;
    double interval = 0;

    MyEnqTotals[idx] = 0;
    MyDeqTotals[idx] = 0;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data[dfTEST_CNT];

    for (int i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        Q->Enqueue(&data[i]);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > EnqMax) EnqMax = interval;
        MyEnqTotals[idx] += interval;
    }
    MyEnqTotals[idx] -= EnqMax;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        Q->Dequeue();
        QueryPerformanceCounter(&end);
        
        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > DeqMax) DeqMax = interval;
        MyDeqTotals[idx] += interval;
    }
    MyDeqTotals[idx] -= DeqMax;

    return 0;
}

unsigned __stdcall LockQueueThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    queue<int*>* Q = (queue<int*>*)input->_Q;
    int idx = input->_idx;
    SRWLOCK* lock = input->_lock;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double EnqMax = 0;
    double DeqMax = 0;
    double interval = 0;

    LockEnqTotals[idx] = 0;
    LockDeqTotals[idx] = 0;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data[dfTEST_CNT];

    for (int i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(lock);
        Q->push(&data[i]);
        ReleaseSRWLockExclusive(lock);

        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > EnqMax) EnqMax = interval;
        LockEnqTotals[idx] += interval;
    }
    LockEnqTotals[idx] -= EnqMax;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(lock);
        Q->pop();
        ReleaseSRWLockExclusive(lock);

        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > DeqMax) DeqMax = interval;
        LockDeqTotals[idx] += interval;
    }
    LockDeqTotals[idx] -= DeqMax;

    return 0;
}

unsigned __stdcall StlQueueThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    concurrent_queue<int*>* Q = (concurrent_queue<int*>*)input->_Q;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double EnqMax = 0;
    double DeqMax = 0;
    double interval = 0;

    StlEnqTotals[idx] = 0;
    StlDeqTotals[idx] = 0;
    int data[dfTEST_CNT];

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        Q->push(&data[i]);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > EnqMax) EnqMax = interval;
        StlEnqTotals[idx] += interval;
    }
    StlEnqTotals[idx] -= EnqMax;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        int* tmp;
        QueryPerformanceCounter(&start);
        Q->try_pop(tmp);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > DeqMax) DeqMax = interval;
        StlDeqTotals[idx] += interval;
    }
    StlDeqTotals[idx] -= DeqMax;

    return 0;
}

void comparePerformance()
{
    // Test My Lock-Free Queue
    CLockFreeQueue<int*> LockFreeQueue;
    HANDLE MyThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&LockFreeQueue, i);
        MyThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeQueueThread, arg, 0, nullptr);
        if (MyThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, MyThreads, true, INFINITE);

    double MyEnqTotal = 0;
    double MyDeqTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        MyEnqTotal += MyEnqTotals[i];
        MyDeqTotal += MyDeqTotals[i];
    }
    MyEnqDuration = MyEnqTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    MyDeqDuration = MyDeqTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);

    // Test Queue with Lock
    SRWLOCK lock;
    InitializeSRWLock(&lock);
    queue<int*> LockQueue;
    HANDLE LockThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&LockQueue, i, &lock);
        LockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockQueueThread, arg, 0, nullptr);
        if (LockThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, LockThreads, true, INFINITE);

    double LockEnqTotal = 0;
    double LockDeqTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        LockEnqTotal += LockEnqTotals[i];
        LockDeqTotal += LockDeqTotals[i];
    }
    LockEnqDuration = LockEnqTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    LockDeqDuration = LockDeqTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);

    // Test Stl Queue
    concurrent_queue<int*> StlQueue;
    HANDLE StlThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&StlQueue, i);
        StlThreads[i] = (HANDLE)_beginthreadex(NULL, 0, StlQueueThread, (void*)arg, 0, nullptr);
        if (StlThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, StlThreads, true, INFINITE);

    double StlEnqTotal = 0;
    double StlDeqTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        StlEnqTotal += StlEnqTotals[i];
        StlDeqTotal += StlDeqTotals[i];
    }
    StlEnqDuration = StlEnqTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    StlDeqDuration = StlDeqTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
}

void Test()
{
    beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (beginThreadComplete == NULL) printf("Error\n");

    timeBeginPeriod(1);
    comparePerformance();  
    timeEndPeriod(1);

    ::printf("My Queue: %f, %f\n", MyEnqDuration, MyDeqDuration);
    ::printf("Lock Queue: %f, %f\n", LockEnqDuration, LockDeqDuration);
    ::printf("Stl Queue: %f, %f\n", StlEnqDuration, StlDeqDuration);
    ::printf("\n");
}