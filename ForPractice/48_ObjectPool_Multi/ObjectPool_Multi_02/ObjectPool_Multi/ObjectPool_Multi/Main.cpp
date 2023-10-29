#include "CProfiler.h"
#include "CObjectPool.h"
#include "CLockFreePool.h"
#include "CTlsObjectPool.h"
#include <process.h>

#define BLOCK1	1
#define BLOCK2	4
#define BLOCK3	16
#define BLOCK4	64
#define BLOCK5	128

struct Data1 { int data; };
struct Data2 { int data[BLOCK2]; };
struct Data3 { int data[BLOCK3]; };
struct Data4 { int data[BLOCK4]; };
struct Data5 { int data[BLOCK5]; };

#define THREAD_CNT 8
#define TEST_CNT 10000
#define TOTAL_CNT 80000
#define LOOP_CNT 10

long g_profileComplete = 0;
HANDLE g_beginThreadComplete = nullptr;
HANDLE g_printComplete = nullptr;
unsigned __stdcall NewDelete5Thread(void* arg);
unsigned __stdcall BasicLockPool5Thread(void* arg);
unsigned __stdcall LockFreePool5Thread(void* arg);
unsigned __stdcall TlsLockPool5Thread(void* arg);

int main()
{
    // Setting =========================================

    PRO_SET();
    for (int i = 0; i < TOTAL_CNT; i++)
    {
        Data5* data5 = new Data5;
        delete data5;
    }

    g_printComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_printComplete == NULL) return 0;
    g_beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_beginThreadComplete == NULL) return 0;

    // Single New-Delete Test =========================================
    
    ::printf("Single New-Delete Test");
    Data5** tmp5 = new Data5* [TOTAL_CNT];

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TOTAL_CNT; i++)
        {
            PRO_BEGIN(L"Single New");
            Data5* data5 = new Data5;
            PRO_END(L"Single New");
            tmp5[i] = data5;
        }

        for (int i = 0; i < TOTAL_CNT; i++)
        {
            PRO_BEGIN(L"Single Delete");
            delete tmp5[i];
            PRO_END(L"Single Delete");
        }
    }

    PRO_PRINT_ADDUP();
    PRO_RESET_ADDUP();
    PRO_RESET();
    delete[] tmp5;

    // Multi New-Delete Test =========================================

    ::printf("Multi New-Delete Test");
    HANDLE NewDelete5Threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        NewDelete5Threads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDelete5Thread, nullptr, 0, nullptr);
        if (NewDelete5Threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, NewDelete5Threads, true, INFINITE);

    // Basic Lock Pool Test =========================================
    
    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CObjectPool<Data5>* _pBasicLockPool5 = new CObjectPool<Data5>(TOTAL_CNT, false);

    ::printf("Basic Lock Pool Test\n");
    HANDLE BasicLockPool5Threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        BasicLockPool5Threads[i] = (HANDLE)_beginthreadex(NULL, 0, BasicLockPool5Thread, _pBasicLockPool5, 0, nullptr);
        if (BasicLockPool5Threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, BasicLockPool5Threads, true, INFINITE);

    // Lock Free Pool Test =========================================

    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CLockFreePool<Data5>* _pLockFreePool5 = new CLockFreePool<Data5>(TOTAL_CNT, false);

    ::printf("Basic Lock Pool Test\n");
    HANDLE LockFreePool5Threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        LockFreePool5Threads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePool5Thread, _pLockFreePool5, 0, nullptr);
        if (LockFreePool5Threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, LockFreePool5Threads, true, INFINITE);

    // Tls Lock Pool Test =========================================

    g_profileComplete = 0;
    ResetEvent(g_printComplete);
    ResetEvent(g_beginThreadComplete);
    CTlsObjectPool<Data5>* _pTlsLockPool5 = new CTlsObjectPool<Data5>(TOTAL_CNT, false);

    ::printf("Tls Lock Pool Test\n");
    HANDLE TlsLockPool5Threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        TlsLockPool5Threads[i] = (HANDLE)_beginthreadex(NULL, 0, TlsLockPool5Thread, _pTlsLockPool5, 0, nullptr);
        if (TlsLockPool5Threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    SetEvent(g_beginThreadComplete);
    WaitForMultipleObjects(THREAD_CNT, TlsLockPool5Threads, true, INFINITE);

    return 0;
}

unsigned __stdcall NewDelete5Thread(void* arg)
{
    PRO_SET();
    Data5** tmp5 = new Data5 * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Multi New");
            Data5* data5 = new Data5;
            PRO_END(L"Multi New");
            tmp5[i] = data5;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Multi Delete");
            delete tmp5[i];
            PRO_END(L"Multi Delete");
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        PRO_PRINT_ADDUP();
        PRO_RESET_ADDUP();
        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    PRO_RESET();
    delete[] tmp5;
    return 0;
}

unsigned __stdcall BasicLockPool5Thread(void* arg)
{
    CObjectPool<Data5>* pBasicLockPool5 = (CObjectPool<Data5>*)arg;

    PRO_SET();
    Data5** tmp5 = new Data5 * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Basic Pool Alloc");
            Data5* data5 = pBasicLockPool5->Alloc();
            PRO_END(L"Basic Pool Alloc");
            tmp5[i] = data5;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Basic Pool Free");
            pBasicLockPool5->Free(tmp5[i]);
            PRO_END(L"Basic Pool Free");
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        PRO_PRINT_ADDUP();
        PRO_RESET_ADDUP();
        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    PRO_RESET();
    delete[] tmp5;
    return 0;
}

unsigned __stdcall LockFreePool5Thread(void* arg)
{
    CLockFreePool<Data5>* pLockFreePool5 = (CLockFreePool<Data5>*)arg;

    PRO_SET();
    Data5** tmp5 = new Data5 * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Lock Free Alloc");
            Data5* data5 = pLockFreePool5->Alloc();
            PRO_END(L"Lock Free Alloc");
            tmp5[i] = data5;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Lock Free Free");
            pLockFreePool5->Free(tmp5[i]);
            PRO_END(L"Lock Free Free");
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        PRO_PRINT_ADDUP();
        PRO_RESET_ADDUP();
        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    PRO_RESET();
    delete[] tmp5;
    return 0;
}

unsigned __stdcall TlsLockPool5Thread(void* arg)
{
    CTlsObjectPool<Data5>* pTlsPool5 = (CTlsObjectPool<Data5>*)arg;

    PRO_SET();
    Data5** tmp5 = new Data5 * [TEST_CNT];
    WaitForSingleObject(g_beginThreadComplete, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Tls Pool Alloc");
            Data5* data5 = pTlsPool5->Alloc();
            PRO_END(L"Tls Pool Alloc");
            tmp5[i] = data5;
        }

        for (int i = 0; i < TEST_CNT; i++)
        {
            PRO_BEGIN(L"Tls Pool Free");
            pTlsPool5->Free(tmp5[i]);
            PRO_END(L"Tls Pool Free");
        }
    }

    long ret = InterlockedIncrement(&g_profileComplete);
    if (ret == THREAD_CNT)
    {
        PRO_PRINT_ADDUP();
        PRO_RESET_ADDUP();
        SetEvent(g_printComplete);
    }
    else
    {
        WaitForSingleObject(g_printComplete, INFINITE);
    }

    PRO_RESET();
    delete[] tmp5;
    return 0;
}

