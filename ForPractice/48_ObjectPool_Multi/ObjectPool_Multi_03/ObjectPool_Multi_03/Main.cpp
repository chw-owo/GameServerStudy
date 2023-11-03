#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <queue>
#include <set>

#include "CObjectPool.h"
#include "CLockPool.h"
#include "CLockFreePool.h"
#include "CTlsObjectPool.h"
using namespace std;

#define DELAY 0.00001
#define BLOCK 8
struct Data { char data[BLOCK]; };

#define POOL_CNT 4
#define THREAD_CNT 4
#define TEST_CNT 10000
#define TOTAL_CNT 40000
#define LOOP_CNT 10
#define MS_PER_SEC 1000000

long g_profileComplete = 0;
HANDLE g_beginThreadComplete = nullptr;
HANDLE g_printComplete = nullptr;

int newCalls[POOL_CNT][THREAD_CNT];
int deleteCalls[POOL_CNT][THREAD_CNT];
double newTimes[POOL_CNT][THREAD_CNT];
double deleteTimes[POOL_CNT][THREAD_CNT];

void SingleNewDelete();
void SingleBasicPool();
void SignleBasicLockPool();
unsigned __stdcall NewDeleteThread(void* arg);
unsigned __stdcall BasicLockPoolThread(void* arg);
unsigned __stdcall LockFreePoolThread(void* arg);
unsigned __stdcall TlsLockPoolThread(void* arg);

set<unsigned __int64> address[6][THREAD_CNT];
inline unsigned __int64 GetAddress(Data* p)
{
    return ((unsigned __int64)p) & 0xffffffffffffffc0;
}

struct argForThread
{
    argForThread(size_t pool, int idx) : _pool(pool), _idx(idx) {};    
    size_t _pool;
    int _idx;
};

int main()
{
    Data** tmp = new Data * [TEST_CNT];
    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            Data* data = new Data;
            tmp[i] = data;
        }
        for (int i = 0; i < TEST_CNT; i++)
        {
            delete tmp[i];
        }
    }
    delete[] tmp;

    SingleNewDelete();
    SingleBasicPool();
    // SignleBasicLockPool();
    
    g_printComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_printComplete == NULL) return 0;
    g_beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_beginThreadComplete == NULL) return 0;
    
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
    CLockPool<Data>* _pBasicLockPool = new CLockPool<Data>(TOTAL_CNT, false);
    
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
    delete _pBasicLockPool;

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
    delete _pLockFreePool;

    // Tls Lock Pool Test =========================================

    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CTlsObjectPool<Data>* _pTlsLockPool = new CTlsObjectPool<Data>(TEST_CNT * 2, false);

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
    delete _pTlsLockPool;

    return 0;
}

void SingleNewDelete()
{
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;    
    
    __int64 newDelay = 0;
    __int64 deleteDelay = 0;
    int newCall = 0;
    int deleteCall = 0;
    double newTime = 0;
    double deleteTime = 0;
    double newMax = 0;
    double deleteMax = 0;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = new Data;
            QueryPerformanceCounter(&end);

            address[0][0].insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            if (interval > DELAY) newDelay++;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            delete tmp[i];
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > deleteMax) deleteMax = interval;
            if (interval > DELAY) deleteDelay++;
            deleteTime += interval;
            deleteCall++;
        }
    }

    newTime -= newMax;
    deleteTime -= deleteMax;
    newCall--;
    deleteCall--;

    ::printf("Single New-Delete Test\n");
    //::printf("delay: %lld, %lld\n", newDelay, deleteDelay);
    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", address[0][0].size());
    ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
}

void SingleBasicPool()
{
    CObjectPool<Data>* _pBasicPool = new CObjectPool<Data>(TOTAL_CNT, false);

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    __int64 newDelay = 0;
    __int64 deleteDelay = 0;
    int newCall = 0;
    int deleteCall = 0;
    double newTime = 0;
    double deleteTime = 0;
    double newMax = 0;
    double deleteMax = 0;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = _pBasicPool->Alloc();
            QueryPerformanceCounter(&end);

            address[1][0].insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            if (interval > DELAY) newDelay++;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            _pBasicPool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > deleteMax) deleteMax = interval;
            if (interval > DELAY) deleteDelay++;
            deleteTime += interval;
            deleteCall++;
        }
    }

    newTime -= newMax;
    deleteTime -= deleteMax;
    newCall--;
    deleteCall--;

    ::printf("Single Object Pool Test\n");
    //::printf("delay: %lld, %lld\n", newDelay, deleteDelay);
    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", address[1][0].size());
    ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
    delete _pBasicPool;
}

void SignleBasicLockPool()
{
    // SRW가 오래 걸리나 보기 위해 싱글에서 LockPool 테스트 진행
    // 거의 차이나지 않음 그 문제는 아닌 걸로...

    CLockPool<Data>* _pLockPool = new CLockPool<Data>(TOTAL_CNT, false);

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval = 0;

    int newCall = 0;
    int deleteCall = 0;
    double newTime = 0;
    double deleteTime = 0;
    double newMax = 0;
    double deleteMax = 0;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = _pLockPool->Alloc();
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            _pLockPool->Free(tmp[i]);
            QueryPerformanceCounter(&end);

            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > deleteMax) deleteMax = interval;
            deleteTime += interval;
            deleteCall++;
        }
    }

    newTime -= newMax;
    deleteTime -= deleteMax;
    newCall--;
    deleteCall--;

    ::printf("Single Lock Pool Test\n");
    ::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    ::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
    delete _pLockPool;
}

unsigned __stdcall NewDeleteThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    __int64 newDelay = 0;
    __int64 deleteDelay = 0;
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

            address[2][idx].insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            if (interval > DELAY) newDelay++;
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
            if (interval > deleteMax) deleteMax = interval;
            if (interval > DELAY) deleteDelay++;
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
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[0][i];
            deleteCall += deleteCalls[0][i];
            newTime += newTimes[0][i];
            deleteTime += deleteTimes[0][i];

            set<unsigned __int64>::iterator it = address[2][i].begin();
            for (; it != address[2][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Multi New-Delete Test\n");
        //::printf("delay: %lld, %lld\n", newDelay, deleteDelay);
        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
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
    CLockPool<Data>* pool = (CLockPool<Data>*)input->_pool;
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    __int64 newDelay = 0;
    __int64 deleteDelay = 0;
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

            address[3][idx].insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            if (interval > DELAY) newDelay++;
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
            if (interval > deleteMax) deleteMax = interval;
            if (interval > DELAY) deleteDelay++;
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
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[1][i];
            deleteCall += deleteCalls[1][i];
            newTime += newTimes[1][i];
            deleteTime += deleteTimes[1][i];
            set<unsigned __int64>::iterator it = address[3][i].begin();
            for (; it != address[3][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Basic Object Pool Test\n");
        //::printf("delay: %lld, %lld\n", newDelay, deleteDelay);
        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
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

    __int64 newDelay = 0;
    __int64 deleteDelay = 0;
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

            address[4][idx].insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            if (interval > DELAY) newDelay++;
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
            if (interval > deleteMax) deleteMax = interval;
            if (interval > DELAY) deleteDelay++;
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
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[2][i];
            deleteCall += deleteCalls[2][i];
            newTime += newTimes[2][i];
            deleteTime += deleteTimes[2][i];
            set<unsigned __int64>::iterator it = address[3][i].begin();
            for (; it != address[3][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Lock Free Pool Test\n");
        //::printf("delay: %lld, %lld\n", newDelay, deleteDelay);
        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
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
    pool->RegisterThread(true);
    int idx = input->_idx;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    __int64 newDelay = 0;
    __int64 deleteDelay = 0;
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

            address[5][idx].insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            if (interval > DELAY) newDelay++;
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
            if (interval > deleteMax) deleteMax = interval;
            if (interval > DELAY) deleteDelay++;
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
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[3][i];
            deleteCall += deleteCalls[3][i];
            newTime += newTimes[3][i];
            deleteTime += deleteTimes[3][i];
            set<unsigned __int64>::iterator it = address[3][i].begin();
            for (; it != address[3][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Tls Object Pool Test\n");
        //::printf("delay: %lld, %lld\n", newDelay, deleteDelay);
        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
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

