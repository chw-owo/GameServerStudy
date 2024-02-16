#pragma once

#include <Windows.h>
#include <process.h>  
#include <iostream>
#pragma comment(lib, "winmm.lib")

#include "CLockFreeQueue.h"
#include "CBasicQueue.h"
#include <queue>
using namespace std;

#define dfENQ_THREAD_CNT 10
#define dfDEQ_THREAD_CNT 10

#define dfENQ_TEST_CNT 2500000
#define dfDEQ_TEST_CNT 2500000

#define NS_PER_SEC 1000000000
#define McS_PER_SEC 1000000
#define MS_PER_SEC 1000

LARGE_INTEGER freq;
SRWLOCK lock;
__int64 input = 100;

HANDLE MyEnqueueEvent;
HANDLE MyDequeueEvent;
HANDLE MyTotalEvent;
HANDLE LockEnqueueEvent;
HANDLE LockDequeueEvent;
HANDLE LockTotalEvent;

double MyEnqueueTimes[dfENQ_THREAD_CNT] = { 0, };
double MyDequeueTimes[dfDEQ_THREAD_CNT] = { 0, };
double LockEnqueueTimes[dfENQ_THREAD_CNT] = { 0, };
double LockDequeueTimes[dfDEQ_THREAD_CNT] = { 0, };

int MyEnqueueCalls[dfENQ_THREAD_CNT] = { 0, };
int MyDequeueCalls[dfDEQ_THREAD_CNT] = { 0, };
int LockEnqueueCalls[dfENQ_THREAD_CNT] = { 0, };
int LockDequeueCalls[dfDEQ_THREAD_CNT] = { 0, };

double MyEnqueueAvgs[dfENQ_THREAD_CNT] = { 0, };
double MyDequeueAvgs[dfDEQ_THREAD_CNT] = { 0, };
double LockEnqueueAvgs[dfENQ_THREAD_CNT] = { 0, };
double LockDequeueAvgs[dfDEQ_THREAD_CNT] = { 0, };

double MyEnqueueAvg = 0;
double MyDequeueAvg = 0;
double LockEnqueueAvg = 0;
double LockDequeueAvg = 0;

double MyEnqueueStartTimes[dfENQ_THREAD_CNT] = { 0, };
double MyDequeueStartTimes[dfDEQ_THREAD_CNT] = { 0, };
double LockEnqueueStartTimes[dfENQ_THREAD_CNT] = { 0, };
double LockDequeueStartTimes[dfDEQ_THREAD_CNT] = { 0, };

double MyEnqueueEndTimes[dfENQ_THREAD_CNT] = { 0, };
double MyDequeueEndTimes[dfDEQ_THREAD_CNT] = { 0, };
double LockEnqueueEndTimes[dfENQ_THREAD_CNT] = { 0, };
double LockDequeueEndTimes[dfDEQ_THREAD_CNT] = { 0, };

double MyEnqueueTotal = 0;
double MyDequeueTotal = 0;
double LockEnqueueTotal = 0;
double LockDequeueTotal = 0;

queue<__int64> BasicQueue;
// CBasicQueue<__int64>* BasicQueue;
CLockFreeQueue<__int64>* LockFreeQueue;

unsigned __stdcall LockFreeEnqueueThread(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    CLockFreeQueue<int*>::_pQueuePool->Initialize();
    // WaitForSingleObject(&MyTotalEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    MyEnqueueStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfENQ_TEST_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        LockFreeQueue->Enqueue(input);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyEnqueueTimes[idx] += interval;
        MyEnqueueCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    MyEnqueueEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    MyDequeueStartTimes[idx] = total_start.QuadPart;

    int i = 0;
    for (; i < dfDEQ_TEST_CNT;)
    {
        QueryPerformanceCounter(&start);
        LockFreeQueue->Dequeue();
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyDequeueTimes[idx] += interval;
        MyDequeueCalls[idx]++;
        i++;
    }

    QueryPerformanceCounter(&total_end);
    MyDequeueEndTimes[idx] = total_end.QuadPart;

    return 0;
}

unsigned __stdcall LockEnqueueThread(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    // WaitForSingleObject(&LockTotalEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    LockEnqueueStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfENQ_TEST_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        BasicQueue.push(input);
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockEnqueueTimes[idx] += interval;
        LockEnqueueCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    LockEnqueueEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    LockDequeueStartTimes[idx] = total_start.QuadPart;

    int i = 0;
    for (; i < dfDEQ_TEST_CNT;)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(&lock);
        if (!BasicQueue.empty())
        {
            BasicQueue.pop();    
        }
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockDequeueTimes[idx] += interval;
        LockDequeueCalls[idx]++;
        i++;
    }

    QueryPerformanceCounter(&total_end);
    LockDequeueEndTimes[idx] = total_end.QuadPart;

    return 0;
}


unsigned __stdcall LockFreeEnqueueThread2(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    CLockFreeQueue<int*>::_pQueuePool->Initialize();
    // WaitForSingleObject(&MyTotalEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    MyDequeueStartTimes[idx] = total_start.QuadPart;

    int i = 0;
    for (; i < dfDEQ_TEST_CNT;)
    {
        QueryPerformanceCounter(&start);
        LockFreeQueue->Dequeue();
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyDequeueTimes[idx] += interval;
        MyDequeueCalls[idx]++;
        i++;
    }

    QueryPerformanceCounter(&total_end);
    MyDequeueEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    MyEnqueueStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfENQ_TEST_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        LockFreeQueue->Enqueue(input);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyEnqueueTimes[idx] += interval;
        MyEnqueueCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    MyEnqueueEndTimes[idx] = total_end.QuadPart;

    return 0;
}

unsigned __stdcall LockEnqueueThread2(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    // WaitForSingleObject(&LockTotalEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    LockDequeueStartTimes[idx] = total_start.QuadPart;

    int i = 0;
    for (; i < dfDEQ_TEST_CNT;)
    {
        QueryPerformanceCounter(&start);

        AcquireSRWLockExclusive(&lock);
        if (!BasicQueue.empty())
        {
            BasicQueue.pop();
        }
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockDequeueTimes[idx] += interval;
        LockDequeueCalls[idx]++;
        i++;
    }

    QueryPerformanceCounter(&total_end);
    LockDequeueEndTimes[idx] = total_end.QuadPart;


    QueryPerformanceCounter(&total_start);
    LockEnqueueStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfENQ_TEST_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        BasicQueue.push(input);
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockEnqueueTimes[idx] += interval;
        LockEnqueueCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    LockEnqueueEndTimes[idx] = total_end.QuadPart;

    return 0;
}

void comparePerformance()
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    // Test Lock-Free Queue =======================================================================

    HANDLE H_MyEnqueueThread[dfENQ_THREAD_CNT/2];
    for (int i = 0; i < dfENQ_THREAD_CNT/2; i++)
    {
        H_MyEnqueueThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeEnqueueThread, (void*)i, 0, nullptr);
        if (H_MyEnqueueThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    // SetEvent(MyEnqueueEvent);
    // WaitForMultipleObjects(dfTHREAD_CNT, H_MyEnqueueThread, true, INFINITE);

    HANDLE H_MyDequeueThread[dfDEQ_THREAD_CNT /2];
    for (int i = 0; i < dfDEQ_THREAD_CNT/2; i++)
    {
        H_MyDequeueThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreeEnqueueThread2, (void*)(i + (dfDEQ_THREAD_CNT / 2)), 0, nullptr);
        if (H_MyDequeueThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(MyTotalEvent);
    WaitForMultipleObjects(dfDEQ_THREAD_CNT/2, H_MyDequeueThread, true, INFINITE);
    WaitForMultipleObjects(dfENQ_THREAD_CNT/2, H_MyEnqueueThread, true, INFINITE);

    // Test Lock Queue =======================================================================

    HANDLE H_LockEnqueueThread[dfENQ_THREAD_CNT/2];
    for (int i = 0; i < dfENQ_THREAD_CNT/2; i++)
    {
        H_LockEnqueueThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockEnqueueThread, (void*)i, 0, nullptr);
        if (H_LockEnqueueThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    // SetEvent(LockEnqueueEvent);
    // WaitForMultipleObjects(dfTHREAD_CNT, H_LockEnqueueThread, true, INFINITE);

    HANDLE H_LockDequeueThread[dfDEQ_THREAD_CNT /2];
    for (int i = 0; i < dfDEQ_THREAD_CNT/2; i++)
    {
        H_LockDequeueThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockEnqueueThread2, (void*)(i + (dfDEQ_THREAD_CNT / 2)), 0, nullptr);
        if (H_LockDequeueThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    
    SetEvent(LockTotalEvent);
    WaitForMultipleObjects(dfDEQ_THREAD_CNT/2, H_LockDequeueThread, true, INFINITE);
    WaitForMultipleObjects(dfENQ_THREAD_CNT/2, H_LockEnqueueThread, true, INFINITE);
}

void printoutResult()
{
    ::printf("Test Count: %d\n", dfENQ_TEST_CNT);
    ::printf("Enq Thread Count: %d\n", dfENQ_THREAD_CNT);
    ::printf("Deq Thread Count: %d\n", dfDEQ_THREAD_CNT);
    ::printf("Unit: milli-second\n\n");

    double MyEnqueueStartTime = DBL_MAX;
    double MyEnqueueEndTime = -DBL_MAX;
    double MyDequeueStartTime = DBL_MAX;
    double MyDequeueEndTime = -DBL_MAX;

    for (int i = 0; i < dfENQ_THREAD_CNT; i++)
    {
        if (MyEnqueueStartTime > MyEnqueueStartTimes[i])
            MyEnqueueStartTime = MyEnqueueStartTimes[i];
        if (MyEnqueueEndTime < MyEnqueueEndTimes[i])
            MyEnqueueEndTime = MyEnqueueEndTimes[i];

        MyEnqueueTimes[i] *= MS_PER_SEC;
        if (MyEnqueueCalls[i] == 0) MyEnqueueAvgs[i] = 0;
        else MyEnqueueAvgs[i] = MyEnqueueTimes[i] / MyEnqueueCalls[i];
        MyEnqueueAvg += MyEnqueueAvgs[i];
    }

    for (int i = 0; i < dfDEQ_THREAD_CNT; i++)
    {
        if (MyDequeueStartTime > MyDequeueStartTimes[i])
            MyDequeueStartTime = MyDequeueStartTimes[i];
        if (MyDequeueEndTime < MyDequeueEndTimes[i])
            MyDequeueEndTime = MyDequeueEndTimes[i];

        MyDequeueTimes[i] *= MS_PER_SEC;
        if (MyDequeueCalls[i] == 0)  MyDequeueAvgs[i] = 0;
        else MyDequeueAvgs[i] = MyDequeueTimes[i] / MyDequeueCalls[i];
        MyDequeueAvg += MyDequeueAvgs[i];
    }

    MyEnqueueAvg = MyEnqueueAvg / dfENQ_THREAD_CNT;
    MyDequeueAvg = MyDequeueAvg / dfDEQ_THREAD_CNT;
    MyEnqueueTotal = (MyEnqueueEndTime - MyEnqueueStartTime) / (double)freq.QuadPart;
    MyDequeueTotal = (MyDequeueEndTime - MyDequeueStartTime) / (double)freq.QuadPart;

    ::printf("<Lock-Free Queue>\n");
    ::printf("| Enqueue |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfENQ_THREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, MyEnqueueTimes[i], MyEnqueueCalls[i], MyEnqueueAvgs[i]);
    }
    ::printf("Enqueue Total: %f\n", MyEnqueueTotal * MS_PER_SEC);
    ::printf("\n");
    ::printf("| Dequeue  |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfDEQ_THREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, MyDequeueTimes[i], MyDequeueCalls[i], MyDequeueAvgs[i]);
    }
    ::printf("Dequeue Total: %f\n", MyDequeueTotal * MS_PER_SEC);
    ::printf("\n");

    double LockEnqueueStartTime = DBL_MAX;
    double LockEnqueueEndTime = -DBL_MAX;
    double LockDequeueStartTime = DBL_MAX;
    double LockDequeueEndTime = -DBL_MAX;

    for (int i = 0; i < dfENQ_THREAD_CNT; i++)
    {
        if (LockEnqueueStartTime > LockEnqueueStartTimes[i])
            LockEnqueueStartTime = LockEnqueueStartTimes[i];
        if (LockEnqueueEndTime < LockEnqueueEndTimes[i])
            LockEnqueueEndTime = LockEnqueueEndTimes[i];

        LockEnqueueTimes[i] *= MS_PER_SEC;
        if (LockEnqueueCalls[i] == 0) LockEnqueueAvgs[i] = 0;
        else LockEnqueueAvgs[i] = LockEnqueueTimes[i] / LockEnqueueCalls[i];
        LockEnqueueAvg += LockEnqueueAvgs[i];
    }

    for (int i = 0; i < dfDEQ_THREAD_CNT; i++)
    {
        if (LockDequeueStartTime > LockDequeueStartTimes[i])
            LockDequeueStartTime = LockDequeueStartTimes[i];
        if (LockDequeueEndTime < LockDequeueEndTimes[i])
            LockDequeueEndTime = LockDequeueEndTimes[i];

        LockDequeueTimes[i] *= MS_PER_SEC;
        if (LockDequeueCalls[i] == 0)  LockDequeueAvgs[i] = 0;
        else LockDequeueAvgs[i] = LockDequeueTimes[i] / LockDequeueCalls[i];
        LockDequeueAvg += LockDequeueAvgs[i];
    }

    LockEnqueueAvg = LockEnqueueAvg / dfENQ_THREAD_CNT;
    LockDequeueAvg = LockDequeueAvg / dfDEQ_THREAD_CNT;
    LockEnqueueTotal = (LockEnqueueEndTime - LockEnqueueStartTime) / (double)freq.QuadPart;
    LockDequeueTotal = (LockDequeueEndTime - LockDequeueStartTime) / (double)freq.QuadPart;

    ::printf("<Basic Queue + SRWLock>\n");
    ::printf("| Enqueue |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfENQ_THREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, LockEnqueueTimes[i], LockEnqueueCalls[i], LockEnqueueAvgs[i]);
    }
    ::printf("Enqueue Total: %f\n", LockEnqueueTotal * MS_PER_SEC);
    ::printf("\n");
    ::printf("| Dequeue  |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfDEQ_THREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, LockDequeueTimes[i], LockDequeueCalls[i], LockDequeueAvgs[i]);
    }
    ::printf("Dequeue Total: %f\n", LockDequeueTotal * MS_PER_SEC);
    ::printf("\n");
}

void Test()
{
    // BasicQueue = new CBasicQueue<__int64>;
    LockFreeQueue = new CLockFreeQueue<__int64>;
    InitializeSRWLock(&lock);
    QueryPerformanceFrequency(&freq);

    comparePerformance();
    printoutResult();

    // delete LockFreeQueue;
}
