#include "PoolTest.h"
#include "CTlsObjectPool.h"
#include "CTlsPool.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <queue>
#include <set>
using namespace std;

#define POOL_CNT 3
#define THREAD_CNT 4
#define TEST_CNT 10000
#define TOTAL_CNT 40000
#define LOOP_CNT 10
#define MS_PER_SEC 1000000

long g_profileComplete = 0;
HANDLE g_beginThreadComplete = nullptr;
HANDLE g_printComplete = nullptr;
struct Data { char data[BLOCK]; };

int newCalls[POOL_CNT][THREAD_CNT];
int deleteCalls[POOL_CNT][THREAD_CNT];
double newTimes[POOL_CNT][THREAD_CNT];
double deleteTimes[POOL_CNT][THREAD_CNT];

struct argForThread
{
    argForThread(size_t pool, int idx)
        : _pool(pool), _idx(idx) {};
    size_t _pool;
    int _idx;
};

void SetMultiTest()
{
    g_printComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_printComplete == NULL) printf("Error\n");
    g_beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_beginThreadComplete == NULL) printf("Error\n");
}

void NewDeletelTest()
{
    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    HANDLE NewDeletelTestThreads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        NewDeletelTestThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeletelTestThread, (void*)i, 0, nullptr);
        if (NewDeletelTestThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, NewDeletelTestThreads, true, INFINITE);
}

unsigned __stdcall NewDeletelTestThread(void* arg)
{
    int idx = (int)arg;
    int poolIdx = 0;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double newMax = 0;
    double deleteMax = 0;
    double interval = 0;
    QueryPerformanceFrequency(&freq);

    Data** tmp = new Data * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = new Data;
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;

            newTimes[poolIdx][idx] += interval;
            newCalls[poolIdx][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            delete tmp[i];
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > deleteMax) deleteMax = interval;

            deleteTimes[poolIdx][idx] += interval;
            deleteCalls[poolIdx][idx]++;
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        int newCall = 0;
        int deleteCall = 0;
        double newTime = 0;
        double deleteTime = 0;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTime += newTimes[poolIdx][i];
            deleteTime += deleteTimes[poolIdx][i];
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Multi New-Delete\n");
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
        ::printf("\n");

        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    delete[] tmp;
    return 0;
}

void TlsLockPoolTest()
{
    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CTlsObjectPool<Data>* pool = new CTlsObjectPool<Data>(TEST_CNT * 2, false);

    HANDLE threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)pool, i);
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, TlsLockPoolTestThread, arg, 0, nullptr);
        if (threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, threads, true, INFINITE);
    delete pool;
}

unsigned __stdcall TlsLockPoolTestThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CTlsObjectPool<Data>* pool = (CTlsObjectPool<Data>*)input->_pool;
    pool->RegisterThread(true);

    int idx = input->_idx;
    int poolIdx = 1;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double newMax = 0;
    double deleteMax = 0;
    double interval = 0;
    QueryPerformanceFrequency(&freq);

    Data** tmp = new Data * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = pool->Alloc();
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;

            newTimes[poolIdx][idx] += interval;
            newCalls[poolIdx][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > deleteMax) deleteMax = interval;

            deleteTimes[poolIdx][idx] += interval;
            deleteCalls[poolIdx][idx]++;
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        int newCall = 0;
        int deleteCall = 0;
        double newTotalTime = 0;
        double deleteTotalTime = 0;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTotalTime += newTimes[poolIdx][i];
            deleteTotalTime += deleteTimes[poolIdx][i];
        }

        newTotalTime -= newMax;
        deleteTotalTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Tls Object Pool\n");
        ::printf("new total average: %.4lfms\n", newTotalTime * MS_PER_SEC / newCall);
        ::printf("delete total average: %.4lfms\n", deleteTotalTime * MS_PER_SEC / deleteCall);
        ::printf("\n");

        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    delete[] tmp;
    return 0;
}

void TlsLockPoolUpgradeTest()
{
    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CTlsPool<Data>* pool = new CTlsPool<Data>(TOTAL_CNT * 2, false);

    HANDLE threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)pool, i);
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, TlsLockPoolUpgradeTestThread, arg, 0, nullptr);
        if (threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, threads, true, INFINITE);
    delete pool;
}

void TlsLockPoolUpgrade_SingleTest()
{
    Data** tmp = new Data * [TEST_CNT];
    CTlsPool<Data>* pool = new CTlsPool<Data>(TOTAL_CNT, false);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            Data* data = pool->Alloc();
            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            pool->Free(tmp[i]);
        }
    }
}

unsigned __stdcall TlsLockPoolUpgradeTestThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CTlsPool<Data>* pool = (CTlsPool<Data>*)input->_pool;
    //pool->Initialize();

    int idx = input->_idx;
    int poolIdx = 2;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double newMax = 0;
    double deleteMax = 0;
    double interval = 0;
    QueryPerformanceFrequency(&freq);

    Data** tmp = new Data * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = pool->Alloc();
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            newTimes[poolIdx][idx] += interval;
            newCalls[poolIdx][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > deleteMax) deleteMax = interval;
            deleteTimes[poolIdx][idx] += interval;
            deleteCalls[poolIdx][idx]++;
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        int newCall = 0;
        int deleteCall = 0;
        double newTotalTime = 0;
        double deleteTotalTime = 0;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTotalTime += newTimes[poolIdx][i];
            deleteTotalTime += deleteTimes[poolIdx][i];
        }

        newTotalTime -= newMax;
        deleteTotalTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Tls Object Pool Upgrade\n");
        ::printf("new total average: %.4lfms\n", newTotalTime * MS_PER_SEC / newCall);
        ::printf("delete total average: %.4lfms\n", deleteTotalTime * MS_PER_SEC / deleteCall);
        ::printf("\n");

        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    delete[] tmp;
    return 0;
}