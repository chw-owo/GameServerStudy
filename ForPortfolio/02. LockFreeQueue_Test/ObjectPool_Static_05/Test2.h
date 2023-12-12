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

#define dfTHREAD_CNT 1
#define dfTEST_TOTAL_CNT 5000000
#define MS_PER_SEC 1000000000
#define McS_PER_SEC 1000000

long cnt = 0;
int* input = nullptr;
LARGE_INTEGER freq;

CRITICAL_SECTION lock;
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
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        LockFreeQ->Enqueue(input);      
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyEnqTimes[0] += interval;
        MyEnqCalls[0]++;
        loop++;
    }

    return 0;
}

unsigned __stdcall LockFreeDeqThread(void* arg)
{
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        int* check = LockFreeQ->Dequeue();  
        QueryPerformanceCounter(&end);

        if (check) continue;
        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyDeqTimes[0] += interval;
        MyDeqCalls[0]++;
        loop++;
    }

    return 0;
}

unsigned __stdcall LockEnqThread(void* arg)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        EnterCriticalSection(&lock);
        LockQ.push(input);
        LeaveCriticalSection(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockEnqTimes[0] += interval;
        LockEnqCalls[0]++;
        loop++;
    }

    return 0;
}

unsigned __stdcall LockDeqThread(void* arg)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        bool check = true;
        QueryPerformanceCounter(&start);
        EnterCriticalSection(&lock);
        if (LockQ.size() > 0) LockQ.pop();
        else check = false;
        LeaveCriticalSection(&lock);
        QueryPerformanceCounter(&end);

        if (!check) continue;
        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockDeqTimes[0] += interval;
        LockDeqCalls[0]++;
        loop++;
    }

    return 0;
}

unsigned __stdcall StlEnqThread(void* arg)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        QueryPerformanceCounter(&start);
        StlQ.push(input);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlEnqTimes[0] += interval;
        StlEnqCalls[0]++;
        loop++;
    }

    return 0;
}

unsigned __stdcall StlDeqThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        int* tmp;
        QueryPerformanceCounter(&start);
        bool check = StlQ.try_pop(tmp);
        QueryPerformanceCounter(&end);

        if (!check) continue;
        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        StlDeqTimes[0] += interval;
        StlDeqCalls[0]++;
        loop++;
    }

    return 0;
}

void comparePerformance()
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    // Test My Lock-Free Queue =======================================================================

    HANDLE H_MyEnQThread;
    HANDLE H_MyDeQThread;
    H_MyEnQThread = (HANDLE)_beginthreadex(NULL, 0, LockFreeEnqThread, nullptr, 0, nullptr);
    if (H_MyEnQThread == NULL)
    {
        ::printf(" Error! %s(%d)\n", __func__, __LINE__);
        __debugbreak();
    }
    H_MyDeQThread = (HANDLE)_beginthreadex(NULL, 0, LockFreeDeqThread, nullptr, 0, nullptr);
    if (H_MyDeQThread == NULL)
    {
        ::printf(" Error! %s(%d)\n", __func__, __LINE__);
        __debugbreak();
    }

    WaitForSingleObject(H_MyEnQThread, INFINITE);
    WaitForSingleObject(H_MyDeQThread, INFINITE);

    // Test Queue with Lock =======================================================================
   
    HANDLE H_LockEnQThread;
    HANDLE H_LockDeQThread;
    H_LockEnQThread = (HANDLE)_beginthreadex(NULL, 0, LockEnqThread, nullptr, 0, nullptr);
    if (H_LockEnQThread == NULL)
    {
        ::printf(" Error! %s(%d)\n", __func__, __LINE__);
        __debugbreak();
    }
    H_LockDeQThread = (HANDLE)_beginthreadex(NULL, 0, LockDeqThread, nullptr, 0, nullptr);
    if (H_LockDeQThread == NULL)
    {
        ::printf(" Error! %s(%d)\n", __func__, __LINE__);
        __debugbreak();
    }

    WaitForSingleObject(H_LockEnQThread, INFINITE);
    WaitForSingleObject(H_LockDeQThread, INFINITE);

    // Test Stl Queue: EnQ =======================================================================

    HANDLE H_StlEnQThread;
    HANDLE H_StlDeQThread;
    H_StlEnQThread = (HANDLE)_beginthreadex(NULL, 0, StlEnqThread, nullptr, 0, nullptr);
    if (H_StlEnQThread == NULL)
    {
        ::printf(" Error! %s(%d)\n", __func__, __LINE__);
        __debugbreak();
    }
    H_StlDeQThread = (HANDLE)_beginthreadex(NULL, 0, StlDeqThread, nullptr, 0, nullptr);
    if (H_StlDeQThread == NULL)
    {
        ::printf(" Error! %s(%d)\n", __func__, __LINE__);
        __debugbreak();
    }

    WaitForSingleObject(H_StlEnQThread, INFINITE);
    WaitForSingleObject(H_StlDeQThread, INFINITE);

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
    //::printf("Enq Avg: %f, Deq Avg: %f\n", MyEnqAvg, MyDeqAvg);
    //::printf("\n\n");

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
    //::printf("Enq Avg: %f, Deq Avg: %f\n", LockEnqAvg, LockDeqAvg);
    //::printf("\n\n");


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
    //::printf("Enq Avg: %f, Deq Avg: %f\n", StlEnqAvg, StlDeqAvg);
    //::printf("\n\n");
}

void Test()
{
    LockFreeQ = new CLockFreeQueue<int*>;
    InitializeCriticalSection(&lock);
    QueryPerformanceFrequency(&freq);

    comparePerformance();
    printoutResult();

    // delete LockFreeQ;
}
