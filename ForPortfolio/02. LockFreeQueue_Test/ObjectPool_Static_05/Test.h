
#pragma once
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

HANDLE beginThreadComplete = nullptr;
HANDLE enqComplete = nullptr;

double MyEnqTimes[dfTHREAD_CNT];
double MyDeqTimes[dfTHREAD_CNT];
double LockEnqTimes[dfTHREAD_CNT];
double LockDeqTimes[dfTHREAD_CNT];
double StlEnqTimes[dfTHREAD_CNT];
double StlDeqTimes[dfTHREAD_CNT];

int MyEnqCalls[dfTHREAD_CNT];
int MyDeqCalls[dfTHREAD_CNT];
int LockEnqCalls[dfTHREAD_CNT];
int LockDeqCalls[dfTHREAD_CNT];
int StlEnqCalls[dfTHREAD_CNT];
int StlDeqCalls[dfTHREAD_CNT];

double MyEnqAvgs[dfTHREAD_CNT];
double MyDeqAvgs[dfTHREAD_CNT];
double LockEnqAvgs[dfTHREAD_CNT];
double LockDeqAvgs[dfTHREAD_CNT];
double StlEnqAvgs[dfTHREAD_CNT];
double StlDeqAvgs[dfTHREAD_CNT];

double MyEnqTotal;
double MyDeqTotal;
double LockEnqTotal;
double LockDeqTotal;
double StlEnqTotal;
double StlDeqTotal;

double MyEnqAvg = 0;
double MyDeqAvg = 0;
double LockEnqAvg = 0;
double LockDeqAvg = 0;
double StlEnqAvg = 0;
double StlDeqAvg = 0;

volatile long MyCnt = -1;
volatile long LockCnt = -1;
volatile long StlCnt = -1;
volatile long MyEnqComplete = 0;
volatile long LockEnqComplete = 0;
volatile long StlEnqComplete = 0;

struct argForThread
{
    argForThread(size_t Q, int idx, SRWLOCK* lock = nullptr)
        : _Q(Q), _idx(idx), _lock(lock) {};
    size_t _Q;
    int _idx;
    SRWLOCK* _lock;
};

unsigned __stdcall LockFreeQueueThread1(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CLockFreeQueue<int*>* Q = (CLockFreeQueue<int*>*)input->_Q;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    MyEnqTimes[idx] = 0;
    MyDeqTimes[idx] = 0;
    MyEnqCalls[idx] = 0;
    MyDeqCalls[idx] = 0;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data;

    for (int i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        Q->Enqueue(&data);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyEnqTimes[idx] += interval;
        MyEnqCalls[idx]++;
    }

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        Q->Dequeue();
        QueryPerformanceCounter(&end);
        
        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyDeqTimes[idx] += interval;
        MyDeqCalls[idx]++;
    }

    return 0;
}

unsigned __stdcall LockQueueThread1(void* arg)
{
    argForThread* input = (argForThread*)arg;
    queue<int*>* Q = (queue<int*>*)input->_Q;
    int idx = input->_idx;
    SRWLOCK* lock = input->_lock;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    LockEnqTimes[idx] = 0;
    LockDeqTimes[idx] = 0;
    LockEnqCalls[idx] = 0;
    LockDeqCalls[idx] = 0;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data;

    for (int i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(lock);
        Q->push(&data);
        ReleaseSRWLockExclusive(lock);

        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockEnqTimes[idx] += interval;
        LockEnqCalls[idx]++;
    }

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(lock);
        Q->pop();
        ReleaseSRWLockExclusive(lock);

        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockDeqTimes[idx] += interval;
        LockDeqCalls[idx]++;
    }

    return 0;
}

unsigned __stdcall StlQueueThread1(void* arg)
{
    argForThread* input = (argForThread*)arg;
    concurrent_queue<int*>* Q = (concurrent_queue<int*>*)input->_Q;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    StlEnqTimes[idx] = 0;
    StlDeqTimes[idx] = 0;
    StlEnqCalls[idx] = 0;
    StlDeqCalls[idx] = 0;

    int data;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        Q->push(&data);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlEnqTimes[idx] += interval;
        StlEnqCalls[idx]++;
    }

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        int* tmp;
        QueryPerformanceCounter(&start);
        Q->try_pop(tmp);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlDeqTimes[idx] += interval;
        StlDeqCalls[idx]++;
    }

    return 0;
}

void comparePerformance1()
{
    // Test My Lock-Free Queue
    CLockFreeQueue<int*> LockFreeQueue;
    HANDLE MyThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&LockFreeQueue, i);
        MyThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeQueueThread1, arg, 0, nullptr);
        if (MyThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, MyThreads, true, INFINITE);

    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        MyEnqTimes[i] *= MS_PER_SEC;
        MyDeqTimes[i] *= MS_PER_SEC;
        MyEnqAvgs[i] = MyEnqTimes[i] / MyEnqCalls[i];
        MyDeqAvgs[i] = MyDeqTimes[i] / MyDeqCalls[i];
        MyEnqAvg += MyEnqAvgs[i];
        MyDeqAvg += MyDeqAvgs[i];
    }

    MyEnqAvg = MyEnqAvg / dfTHREAD_CNT;
    MyDeqAvg = MyDeqAvg / dfTHREAD_CNT;

    // Test Queue with Lock
    SRWLOCK lock;
    InitializeSRWLock(&lock);
    queue<int*> LockQueue;
    HANDLE LockThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&LockQueue, i, &lock);
        LockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockQueueThread1, arg, 0, nullptr);
        if (LockThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, LockThreads, true, INFINITE);

    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        LockEnqTimes[i] *= MS_PER_SEC;
        LockDeqTimes[i] *= MS_PER_SEC;
        LockEnqAvgs[i] = LockEnqTimes[i] / LockEnqCalls[i];
        LockDeqAvgs[i] = LockDeqTimes[i] / LockDeqCalls[i];
        LockEnqAvg += LockEnqAvgs[i];
        LockDeqAvg += LockDeqAvgs[i];
    }

    LockEnqAvg = LockEnqAvg / dfTHREAD_CNT;
    LockDeqAvg = LockDeqAvg / dfTHREAD_CNT;

    // Test Stl Queue
    concurrent_queue<int*> StlQueue;
    HANDLE StlThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&StlQueue, i);
        StlThreads[i] = (HANDLE)_beginthreadex(NULL, 0, StlQueueThread1, (void*)arg, 0, nullptr);
        if (StlThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, StlThreads, true, INFINITE);

    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        StlEnqTimes[i] *= MS_PER_SEC;
        StlDeqTimes[i] *= MS_PER_SEC;
        StlEnqAvgs[i] = StlEnqTimes[i] / StlEnqCalls[i];
        StlDeqAvgs[i] = StlDeqTimes[i] / StlDeqCalls[i];
        StlEnqAvg += StlEnqAvgs[i];
        StlDeqAvg += StlDeqAvgs[i];
    }

    StlEnqAvg = StlEnqAvg / dfTHREAD_CNT;
    StlDeqAvg = StlDeqAvg / dfTHREAD_CNT;
}

unsigned __stdcall LockFreeQueueThread2(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CLockFreeQueue<int*>* Q = (CLockFreeQueue<int*>*)input->_Q;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    MyEnqTimes[idx] = 0;
    MyDeqTimes[idx] = 0;
    MyEnqCalls[idx] = 0;
    MyDeqCalls[idx] = 0;

    CLockFreeQueue<int*>::_pQueuePool->Initialize();
    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data;
    long ret = InterlockedIncrement(&MyCnt);
    while(ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        Q->Enqueue(&data);
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyEnqTimes[idx] += interval;
        MyEnqCalls[idx]++;
        ret = InterlockedIncrement(&MyCnt);
    }

    long complete = InterlockedIncrement(&MyEnqComplete);
    if (complete < dfTHREAD_CNT)
    {
        WaitForSingleObject(enqComplete, INFINITE);
    }
    else
    {
        InterlockedExchange(&MyCnt, dfTEST_TOTAL_CNT);
        SetEvent(enqComplete);
    }

    ret = InterlockedDecrement(&MyCnt);
    while (ret > 0)
    {
        QueryPerformanceCounter(&start);
        Q->Dequeue();
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyDeqTimes[idx] += interval;
        MyDeqCalls[idx]++;
        ret = InterlockedDecrement(&MyCnt);
    }

    return 0;
}

unsigned __stdcall LockQueueThread2(void* arg)
{
    argForThread* input = (argForThread*)arg;
    queue<int*>* Q = (queue<int*>*)input->_Q;
    int idx = input->_idx;
    SRWLOCK* lock = input->_lock;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    LockEnqTimes[idx] = 0;
    LockDeqTimes[idx] = 0;
    LockEnqCalls[idx] = 0;
    LockDeqCalls[idx] = 0;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data;
    long ret = InterlockedIncrement(&LockCnt);

    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(lock);
        Q->push(&data);
        Sleep(0);
        ReleaseSRWLockExclusive(lock);

        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockEnqTimes[idx] += interval;
        LockEnqCalls[idx]++;
        ret = InterlockedIncrement(&LockCnt);
    }

    long complete = InterlockedIncrement(&LockEnqComplete);
    if (complete < dfTHREAD_CNT)
    {
        WaitForSingleObject(enqComplete, INFINITE);
    }
    else
    {
        InterlockedExchange(&LockCnt, dfTEST_TOTAL_CNT);
        SetEvent(enqComplete);
    }

    ret = InterlockedDecrement(&LockCnt);
    while (ret > 0)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(lock);
        Q->pop();
        Sleep(0);
        ReleaseSRWLockExclusive(lock);

        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockDeqTimes[idx] += interval;
        LockDeqCalls[idx]++;
        ret = InterlockedDecrement(&LockCnt);
    }
    return 0;
}

unsigned __stdcall StlQueueThread2(void* arg)
{
    argForThread* input = (argForThread*)arg;
    concurrent_queue<int*>* Q = (concurrent_queue<int*>*)input->_Q;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    StlEnqTimes[idx] = 0;
    StlDeqTimes[idx] = 0;
    StlEnqCalls[idx] = 0;
    StlDeqCalls[idx] = 0;

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    int data;
    long ret = InterlockedIncrement(&StlCnt);

    while (ret < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        Q->push(&data);
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlEnqTimes[idx] += interval;
        StlEnqCalls[idx]++;
        ret = InterlockedIncrement(&StlCnt);
    }

    long complete = InterlockedIncrement(&StlEnqComplete);
    if (complete < dfTHREAD_CNT)
    {
        WaitForSingleObject(enqComplete, INFINITE);
    }
    else
    {
        InterlockedExchange(&StlCnt, dfTEST_TOTAL_CNT);
        SetEvent(enqComplete);
    }

    ret = InterlockedDecrement(&StlCnt);
    while (ret > 0)
    {
        int* tmp;
        QueryPerformanceCounter(&start);
        Q->try_pop(tmp);
        Sleep(0);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlDeqTimes[idx] += interval;
        StlDeqCalls[idx]++;
        ret = InterlockedDecrement(&StlCnt);
    }

    return 0;
}

void comparePerformance2()
{
    // Test My Lock-Free Queue
    CLockFreeQueue<int*> LockFreeQueue;
    HANDLE MyThreads[dfTHREAD_CNT];
    ResetEvent(enqComplete);
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&LockFreeQueue, i);
        MyThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeQueueThread2, arg, 0, nullptr);
        if (MyThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, MyThreads, true, INFINITE);

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

    // Test Queue with Lock
    SRWLOCK lock;
    InitializeSRWLock(&lock);
    queue<int*> LockQueue;
    HANDLE LockThreads[dfTHREAD_CNT];
    ResetEvent(enqComplete);
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&LockQueue, i, &lock);
        LockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockQueueThread2, arg, 0, nullptr);
        if (LockThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, LockThreads, true, INFINITE);

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

    // Test Stl Queue
    concurrent_queue<int*> StlQueue;
    HANDLE StlThreads[dfTHREAD_CNT];
    ResetEvent(enqComplete);
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&StlQueue, i);
        StlThreads[i] = (HANDLE)_beginthreadex(NULL, 0, StlQueueThread2, (void*)arg, 0, nullptr);
        if (StlThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, StlThreads, true, INFINITE);

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
    beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (beginThreadComplete == NULL) printf("Error\n");
    enqComplete = CreateEvent(NULL, true, false, nullptr);
    if (enqComplete == NULL) printf("Error\n");

    timeBeginPeriod(1);
    // comparePerformance1();  
    comparePerformance2();  
    timeEndPeriod(1);
}
