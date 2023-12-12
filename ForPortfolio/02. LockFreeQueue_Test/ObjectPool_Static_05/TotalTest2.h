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

#define dfTEST_TOTAL_CNT 5000000
#define McS_PER_SEC 1000000

long cnt = 0;
int* input = nullptr;
LARGE_INTEGER freq;

CRITICAL_SECTION lock;
queue<int*> LockQ;
concurrent_queue<int*> StlQ;
CLockFreeQueue<int*>* LockFreeQ;

double LockFreeEnQTotal = 0;
double LockFreeDeQTotal = 0;
double LockEnQTotal = 0;
double LockDeQTotal = 0;
double StlEnQTotal = 0;
double StlDeQTotal = 0;

unsigned __stdcall LockFreeEnqThread(void* arg)
{
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    QueryPerformanceCounter(&start);
    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        LockFreeQ->Enqueue(input);
        loop++;
    }
    QueryPerformanceCounter(&end);
    LockFreeEnQTotal = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    return 0;
}

unsigned __stdcall LockFreeDeqThread(void* arg)
{
    CLockFreeQueue<int*>::_pQueuePool->Initialize();

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    QueryPerformanceCounter(&start);
    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        int* check = LockFreeQ->Dequeue();
        if (check) continue;
        loop++;
    }
    QueryPerformanceCounter(&end);
    LockFreeDeQTotal = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    return 0;
}

unsigned __stdcall LockEnqThread(void* arg)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    QueryPerformanceCounter(&start);
    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        EnterCriticalSection(&lock);
        LockQ.push(input);
        LeaveCriticalSection(&lock);
        
        loop++;
    }
    QueryPerformanceCounter(&end);
    LockEnQTotal = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    return 0;
}

unsigned __stdcall LockDeqThread(void* arg)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;


    QueryPerformanceCounter(&start);
    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        bool check = true;
        EnterCriticalSection(&lock);
        if (LockQ.size() > 0) LockQ.pop();
        else check = false;
        LeaveCriticalSection(&lock);
        if (!check) continue;
        loop++;
    }
    QueryPerformanceCounter(&end);
    LockDeQTotal = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    return 0;
}

unsigned __stdcall StlEnqThread(void* arg)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    QueryPerformanceCounter(&start);
    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        StlQ.push(input);
        loop++;
    }

    QueryPerformanceCounter(&end);
    StlEnQTotal = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    return 0;
}

unsigned __stdcall StlDeqThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;


    QueryPerformanceCounter(&start);
    long loop = 0;
    while (loop < dfTEST_TOTAL_CNT)
    {
        int* tmp;
        bool check = StlQ.try_pop(tmp);
        if (!check) continue;
        loop++;
    }
    QueryPerformanceCounter(&end);
    StlDeQTotal = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
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
    ::printf("LockFreeEnQ: %f\n", LockFreeEnQTotal);
    ::printf("LockFreeDeQ: %f\n", LockFreeDeQTotal);
    ::printf("LockEnQ: %f\n", LockEnQTotal);
    ::printf("LockDeQ: %f\n", LockDeQTotal);
    ::printf("StlEnQ: %f\n", StlEnQTotal);
    ::printf("StlDeQ: %f\n", StlDeQTotal);
}

void TotalTest()
{
    LockFreeQ = new CLockFreeQueue<int*>;
    InitializeCriticalSection(&lock);
    QueryPerformanceFrequency(&freq);

    comparePerformance();
    printoutResult();

    // delete LockFreeQ;
}
