#include <windows.h>
#include <process.h>
#include <stdio.h>
#include "CObjectPool.h"
#include "CLockFreePool.h"
#include "CTlsObjectPool.h"

#define BLOCK 16
struct Data { int data[BLOCK]; };

#define POOL_CNT 4
#define THREAD_CNT 8
#define TEST_CNT 10000
#define TOTAL_CNT 80000
#define LOOP_CNT 10
#define MS_PER_SEC 1000000

long g_profileComplete = 0;
HANDLE g_beginThreadComplete = nullptr;
HANDLE g_printComplete = nullptr;
unsigned __stdcall NewDeleteThread(void* arg);
unsigned __stdcall BasicLockPoolThread(void* arg);
unsigned __stdcall LockFreePoolThread(void* arg);
unsigned __stdcall TlsLockPoolThread(void* arg);

struct argForThread
{
    argForThread(size_t pool, int idx) : _pool(pool), _idx(idx) {};
    
    size_t _pool;
    int _idx;
};

int main()
{
    // Setting =========================================

    for (int i = 0; i < TOTAL_CNT; i++)
    {
        Data* data = new Data;
        delete data;
    }

    g_printComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_printComplete == NULL) return 0;
    g_beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_beginThreadComplete == NULL) return 0;

    // Single New-Delete Test =========================================

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    int newCall = 0;
    int deleteCall = 0;
    double newTime = 0;
    double deleteTime = 0;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TOTAL_CNT];

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TOTAL_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = new Data;
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TOTAL_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            delete tmp[i];
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            deleteTime += interval;
            deleteCall++;
        }
    }

    ::printf("Single New-Delete Test\n");
    ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;

    // Multi New-Delete Test =========================================

    HANDLE NewDeleteThreads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        NewDeleteThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeleteThread, (void*)i, 0, nullptr);
        if (NewDeleteThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, NewDeleteThreads, true, INFINITE);

    // Basic Lock Pool Test =========================================

    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CObjectPool<Data>* _pBasicLockPool = new CObjectPool<Data>(TOTAL_CNT, false);
    
    HANDLE BasicLockPoolThreads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)_pBasicLockPool, i);
        BasicLockPoolThreads[i] = (HANDLE)_beginthreadex(NULL, 0, BasicLockPoolThread, arg, 0, nullptr);
        if (BasicLockPoolThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, BasicLockPoolThreads, true, INFINITE);

    // Lock Free Pool Test =========================================

    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CLockFreePool<Data>* _pLockFreePool = new CLockFreePool<Data>(TOTAL_CNT, false);

    HANDLE LockFreePoolThreads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        argForThread* arg = new  argForThread((size_t)_pLockFreePool, i);
        LockFreePoolThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePoolThread, arg, 0, nullptr);
        if (LockFreePoolThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, LockFreePoolThreads, true, INFINITE);

    // Tls Lock Pool Test =========================================

    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CTlsObjectPool<Data>* _pTlsLockPool = new CTlsObjectPool<Data>(TOTAL_CNT, false);

    HANDLE TlsLockPoolThreads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        argForThread* arg = new  argForThread((size_t)_pTlsLockPool, i);
        TlsLockPoolThreads[i] = (HANDLE)_beginthreadex(NULL, 0, TlsLockPoolThread, arg, 0, nullptr);
        if (TlsLockPoolThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, TlsLockPoolThreads, true, INFINITE);

    return 0;
}

int newCalls[POOL_CNT][THREAD_CNT];
int deleteCalls[POOL_CNT][THREAD_CNT];
double newTimes[POOL_CNT][THREAD_CNT];
double deleteTimes[POOL_CNT][THREAD_CNT];

unsigned __stdcall NewDeleteThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
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
            newTimes[0][idx] += interval;
            newCalls[0][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            delete tmp[i];
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            deleteTimes[0][idx] += interval;
            deleteCalls[0][idx]++;
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
            newCall += newCalls[0][i];
            deleteCall += deleteCalls[0][i];
            newTime += newTimes[0][i];
            deleteTime += deleteTimes[0][i];
        }

        ::printf("Multi New-Delete Test\n");
        ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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

unsigned __stdcall BasicLockPoolThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CObjectPool<Data>* pool = (CObjectPool<Data>*)input->_pool;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
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
            newTimes[1][idx] += interval;
            newCalls[1][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            deleteTimes[1][idx] += interval;
            deleteCalls[1][idx]++;
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
            newCall += newCalls[1][i];
            deleteCall += deleteCalls[1][i];
            newTime += newTimes[1][i];
            deleteTime += deleteTimes[1][i];
        }

        ::printf("Basic Object Pool Test\n");
        ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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

unsigned __stdcall LockFreePoolThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CLockFreePool<Data>* pool = (CLockFreePool<Data>*)input->_pool;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
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
            newTimes[2][idx] += interval;
            newCalls[2][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            deleteTimes[2][idx] += interval;
            deleteCalls[2][idx]++;
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
            newCall += newCalls[2][i];
            deleteCall += deleteCalls[2][i];
            newTime += newTimes[2][i];
            deleteTime += deleteTimes[2][i];
        }

        ::printf("Lock Free Pool Test\n");
        ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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

unsigned __stdcall TlsLockPoolThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CTlsObjectPool<Data>* pool = (CTlsObjectPool<Data>*)input->_pool;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
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
            newTimes[3][idx] += interval;
            newCalls[3][idx]++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            deleteTimes[3][idx] += interval;
            deleteCalls[3][idx]++;
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
            newCall += newCalls[3][i];
            deleteCall += deleteCalls[3][i];
            newTime += newTimes[3][i];
            deleteTime += deleteTimes[3][i];
        }

        ::printf("Tls Object Pool Test\n");
        ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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
