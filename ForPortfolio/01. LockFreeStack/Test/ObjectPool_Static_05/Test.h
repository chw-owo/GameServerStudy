#pragma once

#include <Windows.h>
#include <process.h>  
#include <iostream>
#include <stack>
#pragma comment(lib, "winmm.lib")

#include "CLockFreeStack.h"
#include "CBasicStack.h"
using namespace std;

#define dfTHREAD_CNT 10
#define dfTEST_TOTAL_CNT 2500000
#define NS_PER_SEC 1000000000
#define McS_PER_SEC 1000000
#define MS_PER_SEC 1000

LARGE_INTEGER freq;
SRWLOCK lock;
__int64 input = 100;

HANDLE MyPushEvent;
HANDLE MyPopEvent;
HANDLE LockPushEvent;
HANDLE LockPopEvent;

double MyPushTimes[dfTHREAD_CNT] = { 0, };
double MyPopTimes[dfTHREAD_CNT] = { 0, };
double LockPushTimes[dfTHREAD_CNT] = { 0, };
double LockPopTimes[dfTHREAD_CNT] = { 0, };

int MyPushCalls[dfTHREAD_CNT] = { 0, };
int MyPopCalls[dfTHREAD_CNT] = { 0, };
int LockPushCalls[dfTHREAD_CNT] = { 0, };
int LockPopCalls[dfTHREAD_CNT] = { 0, };

double MyPushAvgs[dfTHREAD_CNT] = { 0, };
double MyPopAvgs[dfTHREAD_CNT] = { 0, };
double LockPushAvgs[dfTHREAD_CNT] = { 0, };
double LockPopAvgs[dfTHREAD_CNT] = { 0, };

double MyPushAvg = 0;
double MyPopAvg = 0;
double LockPushAvg = 0;
double LockPopAvg = 0;

double MyPushStartTimes[dfTHREAD_CNT] = { 0, };
double MyPopStartTimes[dfTHREAD_CNT] = { 0, };
double LockPushStartTimes[dfTHREAD_CNT] = { 0, };
double LockPopStartTimes[dfTHREAD_CNT] = { 0, };

double MyPushEndTimes[dfTHREAD_CNT] = { 0, };
double MyPopEndTimes[dfTHREAD_CNT] = { 0, };
double LockPushEndTimes[dfTHREAD_CNT] = { 0, };
double LockPopEndTimes[dfTHREAD_CNT] = { 0, };

double MyPushTotal = 0;
double MyPopTotal = 0;
double LockPushTotal = 0;
double LockPopTotal = 0;

stack<__int64> BasicStack;
//CBasicStack<__int64>* BasicStack;
CLockFreeStack<__int64>* LockFreeStack;

unsigned __stdcall LockFreePushThread(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    // WaitForSingleObject(&MyPopEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    MyPushStartTimes[idx] = total_start.QuadPart;

    for(int i = 0; i < dfTEST_TOTAL_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        LockFreeStack->Push(input);      
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyPushTimes[idx] += interval;
        MyPushCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    MyPushEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    MyPopStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfTEST_TOTAL_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        LockFreeStack->Pop();
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyPopTimes[idx] += interval;
        MyPopCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    MyPopEndTimes[idx] = total_end.QuadPart;

    return 0;
}


unsigned __stdcall LockPushThread(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    // WaitForSingleObject(&LockPopEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    LockPushStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfTEST_TOTAL_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        BasicStack.push(input);
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockPushTimes[idx] += interval;
        LockPushCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    LockPushEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    LockPopStartTimes[idx] = total_start.QuadPart;

    int i = 0;
    for (; i < dfTEST_TOTAL_CNT; )
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        if(BasicStack.size() > 0)
        {
            BasicStack.pop();
        }
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockPopTimes[idx] += interval;
        LockPopCalls[idx]++;
        i++;
    }

    QueryPerformanceCounter(&total_end);
    LockPopEndTimes[idx] = total_end.QuadPart;

    return 0;
}

unsigned __stdcall LockFreePushThread2(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    // WaitForSingleObject(&MyPopEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    MyPopStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfTEST_TOTAL_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        LockFreeStack->Pop();
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyPopTimes[idx] += interval;
        MyPopCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    MyPopEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    MyPushStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfTEST_TOTAL_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        LockFreeStack->Push(input);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        MyPushTimes[idx] += interval;
        MyPushCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    MyPushEndTimes[idx] = total_end.QuadPart;

    return 0;
}


unsigned __stdcall LockPushThread2(void* arg)
{
    int idx = (int)arg;
    double interval = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER total_start;
    LARGE_INTEGER total_end;
    // WaitForSingleObject(&LockPopEvent, INFINITE);

    QueryPerformanceCounter(&total_start);
    LockPopStartTimes[idx] = total_start.QuadPart;

    int i = 0;
    for (; i < dfTEST_TOTAL_CNT; )
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        if (BasicStack.size() > 0)
        {
            BasicStack.pop();
        }
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockPopTimes[idx] += interval;
        LockPopCalls[idx]++;
        i++;
    }

    QueryPerformanceCounter(&total_end);
    LockPopEndTimes[idx] = total_end.QuadPart;

    QueryPerformanceCounter(&total_start);
    LockPushStartTimes[idx] = total_start.QuadPart;

    for (int i = 0; i < dfTEST_TOTAL_CNT; i++)
    {
        QueryPerformanceCounter(&start);
        AcquireSRWLockExclusive(&lock);
        BasicStack.push(input);
        ReleaseSRWLockExclusive(&lock);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        LockPushTimes[idx] += interval;
        LockPushCalls[idx]++;
    }

    QueryPerformanceCounter(&total_end);
    LockPushEndTimes[idx] = total_end.QuadPart;

    return 0;
}


void comparePerformance()
{
    // Test Lock Stack =======================================================================

    HANDLE H_LockPushThread[dfTHREAD_CNT / 2];
    for (int i = 0; i < dfTHREAD_CNT / 2; i++)
    {
        H_LockPushThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockPushThread, (void*)i, 0, nullptr);
        if (H_LockPushThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    HANDLE H_LockPushThread2[dfTHREAD_CNT / 2];
    for (int i = 0; i < dfTHREAD_CNT / 2; i++)
    {
        H_LockPushThread2[i] = (HANDLE)_beginthreadex(NULL, 0, LockPushThread2, (void*)(i + dfTHREAD_CNT / 2), 0, nullptr);
        if (H_LockPushThread2[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    // SetEvent(LockPushEvent);
    // WaitForMultipleObjects(dfTHREAD_CNT, H_LockPushThread, true, INFINITE);

    /*
    HANDLE H_LockPopThread[dfTHREAD_CNT];
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        H_LockPopThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockPopThread, (void*)i, 0, nullptr);
        if (H_LockPopThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    */

    SetEvent(LockPopEvent);
    WaitForMultipleObjects(dfTHREAD_CNT / 2, H_LockPushThread, true, INFINITE);
    WaitForMultipleObjects(dfTHREAD_CNT / 2, H_LockPushThread2, true, INFINITE);

    // Test Lock-Free stack =======================================================================

    HANDLE H_MyPushThread[dfTHREAD_CNT / 2];
    for (int i = 0; i < dfTHREAD_CNT / 2; i++)
    {
        H_MyPushThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePushThread, (void*)i, 0, nullptr);
        if (H_MyPushThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    HANDLE H_MyPushThread2[dfTHREAD_CNT / 2];
    for (int i = 0; i < dfTHREAD_CNT / 2; i++)
    {
        H_MyPushThread2[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePushThread2, (void*)(i + dfTHREAD_CNT / 2), 0, nullptr);
        if (H_MyPushThread2[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    // SetEvent(MyPushEvent);
    // WaitForMultipleObjects(dfTHREAD_CNT, H_MyPushThread, true, INFINITE);

    /*
    HANDLE H_MyPopThread[dfTHREAD_CNT];
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        H_MyPopThread[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePopThread, (void*)i, 0, nullptr);
        if (H_MyPopThread[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    */
    SetEvent(MyPopEvent);
    WaitForMultipleObjects(dfTHREAD_CNT / 2, H_MyPushThread, true, INFINITE);
    WaitForMultipleObjects(dfTHREAD_CNT / 2, H_MyPushThread2, true, INFINITE);
}

void printoutResult()
{
    ::printf("Test Count: %d\n", dfTEST_TOTAL_CNT);
    ::printf("Thread Count: %d\n", dfTHREAD_CNT);
    ::printf("Unit: milli-second\n\n");

    double MyPushStartTime = DBL_MAX;
    double MyPushEndTime = - DBL_MAX;
    double MyPopStartTime = DBL_MAX;
    double MyPopEndTime = -DBL_MAX;

    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (MyPushStartTime > MyPushStartTimes[i])
            MyPushStartTime = MyPushStartTimes[i];
        if (MyPushEndTime < MyPushEndTimes[i])
            MyPushEndTime = MyPushEndTimes[i];

        if (MyPopStartTime > MyPopStartTimes[i])
            MyPopStartTime = MyPopStartTimes[i];
        if (MyPopEndTime < MyPopEndTimes[i])
            MyPopEndTime = MyPopEndTimes[i];

        MyPushTimes[i] *= MS_PER_SEC;
        MyPopTimes[i] *= MS_PER_SEC;
        if (MyPushCalls[i] == 0) MyPushAvgs[i] = 0;
        else MyPushAvgs[i] = MyPushTimes[i] / MyPushCalls[i];
        if (MyPopCalls[i] == 0)  MyPopAvgs[i] = 0;
        else MyPopAvgs[i] = MyPopTimes[i] / MyPopCalls[i];
        MyPushAvg += MyPushAvgs[i];
        MyPopAvg += MyPopAvgs[i];
    }

    MyPushAvg = MyPushAvg / dfTHREAD_CNT;
    MyPopAvg = MyPopAvg / dfTHREAD_CNT;
    MyPushTotal = (MyPushEndTime - MyPushStartTime) / (double)freq.QuadPart;
    MyPopTotal = (MyPopEndTime - MyPopStartTime) / (double)freq.QuadPart;

    ::printf("<Lock-Free Stack>\n");
    ::printf("|    Push    |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, MyPushTimes[i], MyPushCalls[i], MyPushAvgs[i]);
    }
    ::printf("Push Total: %f\n", MyPushTotal * MS_PER_SEC);
    ::printf("\n");
    ::printf("|    Pop     |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, MyPopTimes[i], MyPopCalls[i], MyPopAvgs[i]);
    }
    ::printf("Pop Total: %f\n", MyPopTotal * MS_PER_SEC);
    ::printf("\n");

    double LockPushStartTime = DBL_MAX;
    double LockPushEndTime = -DBL_MAX;
    double LockPopStartTime = DBL_MAX;
    double LockPopEndTime = -DBL_MAX;

    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        if (LockPushStartTime > LockPushStartTimes[i])
            LockPushStartTime = LockPushStartTimes[i];
        if (LockPushEndTime < LockPushEndTimes[i])
            LockPushEndTime = LockPushEndTimes[i];

        if (LockPopStartTime > LockPopStartTimes[i])
            LockPopStartTime = LockPopStartTimes[i];
        if (LockPopEndTime < LockPopEndTimes[i])
            LockPopEndTime = LockPopEndTimes[i];

        LockPushTimes[i] *= MS_PER_SEC;
        LockPopTimes[i] *= MS_PER_SEC;
        if (LockPushCalls[i] == 0) LockPushAvgs[i] = 0;
        else LockPushAvgs[i] = LockPushTimes[i] / LockPushCalls[i];
        if (LockPopCalls[i] == 0)  LockPopAvgs[i] = 0;
        else LockPopAvgs[i] = LockPopTimes[i] / LockPopCalls[i];
        LockPushAvg += LockPushAvgs[i];
        LockPopAvg += LockPopAvgs[i];
    }

    LockPushAvg = LockPushAvg / dfTHREAD_CNT;
    LockPopAvg = LockPopAvg / dfTHREAD_CNT;
    LockPushTotal = (LockPushEndTime - LockPushStartTime) / (double)freq.QuadPart;
    LockPopTotal = (LockPopEndTime - LockPopStartTime) / (double)freq.QuadPart;

    ::printf("<Basic Stack + SRWLock>\n");
    ::printf("|    Push    |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, LockPushTimes[i], LockPushCalls[i], LockPushAvgs[i]);
    }
    ::printf("Push Total: %f\n", LockPushTotal * MS_PER_SEC);
    ::printf("\n");
    ::printf("|    Pop     |  Total  |  Call  |  Avg  |\n");
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        ::printf("| Thread %d | %f | %d | %f |\n", i, LockPopTimes[i], LockPopCalls[i], LockPopAvgs[i]);
    }
    ::printf("Pop Total: %f\n", LockPopTotal * MS_PER_SEC);
    ::printf("\n");
}

void Test()
{
    // BasicStack = new CBasicStack<__int64>;
    LockFreeStack = new CLockFreeStack<__int64>;
    InitializeSRWLock(&lock);
    QueryPerformanceFrequency(&freq);

    comparePerformance();
    printoutResult();

    // delete LockFreeStack;
}
