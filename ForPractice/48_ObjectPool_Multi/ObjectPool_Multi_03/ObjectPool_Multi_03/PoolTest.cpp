#include "PoolTest.h"

void ProfileTlsLockPool()
{
}


void ProfileTlsLockPoolUpgrade()
{
}

inline unsigned __int64 GetAddress(Data* p)
{
    return ((unsigned __int64)p) & 0xffffffffffffffc0;
}

void SetTest()
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
}

void SingleNewDelete()
{
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
    set<unsigned __int64> addressTmp;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = new Data;
            QueryPerformanceCounter(&end);

            addressTmp.insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;

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

            deleteTime += interval;
            deleteCall++;
        }
    }

    newTime -= newMax;
    deleteTime -= deleteMax;
    newCall--;
    deleteCall--;

    ::printf("Single New-Delete Test\n");

    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", addressTmp.size());
    //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
}

void SingleBasicLockPool()
{
    CLockPool<Data>* pool = new CLockPool<Data>(TOTAL_CNT, false);

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
    set<unsigned __int64> addressTmp;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = pool->Alloc();
            QueryPerformanceCounter(&end);

            addressTmp.insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
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

    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", addressTmp.size());
    //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
    delete pool;
}

void SingleLockFreePool()
{
    CLockFreePool<Data>* pool = new CLockFreePool<Data>(TOTAL_CNT, false);

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
    set<unsigned __int64> addressTmp;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = pool->Alloc();
            QueryPerformanceCounter(&end);

            addressTmp.insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
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

    ::printf("Single Lock Free Pool Test\n");

    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", addressTmp.size());
    //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
    delete pool;
}

void SingleTlsLockPool()
{
    CTlsObjectPool<Data>* pool = new CTlsObjectPool<Data>(TOTAL_CNT, false);
    pool->RegisterThread(true);

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
    set<unsigned __int64> addressTmp;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = pool->Alloc();
            QueryPerformanceCounter(&end);

            addressTmp.insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
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

    ::printf("Single Tls Pool Test\n");

    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", addressTmp.size());
    //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
    delete pool;
}

void SingleTlsLockPoolUpgrade()
{
    CTlsObjectPool_Upgrade<Data>* pool = new CTlsObjectPool_Upgrade<Data>(TOTAL_CNT, false);
    pool->RegisterThread(true);

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
    set<unsigned __int64> addressTmp;

    QueryPerformanceFrequency(&freq);
    Data** tmp = new Data * [TEST_CNT];

    for (int j = 0; j < LOOP_CNT * THREAD_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            Data* data = pool->Alloc();
            QueryPerformanceCounter(&end);

            addressTmp.insert(GetAddress(data));
            interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            if (interval > newMax) newMax = interval;
            newTime += interval;
            newCall++;

            tmp[i] = data;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            QueryPerformanceCounter(&start);
            pool->Free(tmp[i]);
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

    ::printf("Single Tls Upgrade Pool Test\n");

    ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
    ::printf("Address Cnt: %lld\n", addressTmp.size());
    //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
    ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
    //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
    ::printf("delete average: %.4lfms\n", deleteTime * MS_PER_SEC / deleteCall);
    ::printf("\n");

    delete[] tmp;
    delete pool;
}

void SetMultiTest()
{
    g_printComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_printComplete == NULL) printf("Error\n");
    g_beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_beginThreadComplete == NULL) printf("Error\n");
}

void MultiNewDelete()
{
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
}

void MultiBasicLockPool()
{
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
}

void MultiLockFreePool()
{
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
}

void MultiTlsLockPool()
{
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
}

void MultiTlsLockPoolUpgrade()
{
    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CTlsObjectPool_Upgrade<Data>* _pTlsLockPoolUpgrade = new CTlsObjectPool_Upgrade<Data>(TEST_CNT * 2, false);

    HANDLE TlsLockPoolUpgradeThreads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        argForThread* arg = new  argForThread((size_t)_pTlsLockPoolUpgrade, i);
        TlsLockPoolUpgradeThreads[i] = (HANDLE)_beginthreadex(NULL, 0, TlsLockPoolUpgradeThread, arg, 0, nullptr);
        if (TlsLockPoolUpgradeThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, TlsLockPoolUpgradeThreads, true, INFINITE);
    delete _pTlsLockPoolUpgrade;
}

unsigned __stdcall NewDeleteThread(void* arg)
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

            address[poolIdx][idx].insert(GetAddress(data));
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
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTime += newTimes[poolIdx][i];
            deleteTime += deleteTimes[poolIdx][i];

            set<unsigned __int64>::iterator it = address[poolIdx][i].begin();
            for (; it != address[poolIdx][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Multi New-Delete Test\n");

        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
        //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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

            address[poolIdx][idx].insert(GetAddress(data));
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
        double newTime = 0;
        double deleteTime = 0;
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTime += newTimes[poolIdx][i];
            deleteTime += deleteTimes[poolIdx][i];
            set<unsigned __int64>::iterator it = address[poolIdx][i].begin();
            for (; it != address[poolIdx][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Basic Object Pool Test\n");

        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
        //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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

            address[poolIdx][idx].insert(GetAddress(data));
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
        double newTime = 0;
        double deleteTime = 0;
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTime += newTimes[poolIdx][i];
            deleteTime += deleteTimes[poolIdx][i];
            set<unsigned __int64>::iterator it = address[poolIdx][i].begin();
            for (; it != address[poolIdx][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Lock Free Pool Test\n");

        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
        //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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
    int poolIdx = 3;

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

            address[poolIdx][idx].insert(GetAddress(data));
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
        double newTime = 0;
        double deleteTime = 0;
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTime += newTimes[poolIdx][i];
            deleteTime += deleteTimes[poolIdx][i];
            set<unsigned __int64>::iterator it = address[poolIdx][i].begin();
            for (; it != address[poolIdx][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Tls Object Pool Test\n");

        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
        //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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

unsigned __stdcall TlsLockPoolUpgradeThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CTlsObjectPool<Data>* pool = (CTlsObjectPool<Data>*)input->_pool;
    pool->RegisterThread(true);
    int idx = input->_idx;
    int poolIdx = 4;

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

            address[poolIdx][idx].insert(GetAddress(data));
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
        double newTime = 0;
        double deleteTime = 0;
        set<unsigned __int64> addressTmp;

        for (int i = 0; i < THREAD_CNT; i++)
        {
            newCall += newCalls[poolIdx][i];
            deleteCall += deleteCalls[poolIdx][i];
            newTime += newTimes[poolIdx][i];
            deleteTime += deleteTimes[poolIdx][i];
            set<unsigned __int64>::iterator it = address[poolIdx][i].begin();
            for (; it != address[poolIdx][i].end(); it++)
            {
                addressTmp.insert(*it);
            }
        }

        newTime -= newMax;
        deleteTime -= deleteMax;
        newCall--;
        deleteCall--;

        ::printf("Upgrade Tls Object Pool Test\n");

        ::printf("Max: %.4lfms, %.4lfms\n", newMax * MS_PER_SEC, deleteMax * MS_PER_SEC);
        ::printf("Address Cnt: %lld\n", addressTmp.size());
        //::printf("new time: %.4lfms\n", newTime * MS_PER_SEC);
        ::printf("new average: %.4lfms\n", newTime * MS_PER_SEC / newCall);
        //::printf("delete time: %.4lfms\n", deleteTime * MS_PER_SEC);
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