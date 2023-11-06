#include "CLockFreeQueue.h"
#include "CLockFreeStack.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>

#define TEST_CNT 1000
#define LOOP_CNT 20
#define THREAD_CNT 8

HANDLE g_ready;
HANDLE g_allComplete;
long g_completeCnt = 0;

void StackTest();
void QueueTest();

int main()
{
    g_ready = CreateEvent(NULL, true, false, nullptr);
    if (g_ready == NULL) __debugbreak();
    g_allComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_ready == NULL) __debugbreak();

   // StackTest();
   QueueTest();
}

unsigned __stdcall StackTestThread(void* arg);
void StackTest()
{
    ResetEvent(g_ready);
    ResetEvent(g_allComplete);
    g_completeCnt = 0;

    HANDLE threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, StackTestThread, (void*)(i + 1), 0, nullptr);
        if (threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(g_ready);
    WaitForMultipleObjects(THREAD_CNT, threads, true, INFINITE);
    ::printf("Stack Test Complete!\n");
}

unsigned __stdcall StackTestThread(void* arg)
{
    int tmp[TEST_CNT];
    for (int i = 0; i < TEST_CNT; i++)
        tmp[i] = (int)arg;
    CLockFreeStack<int> stack;
    WaitForSingleObject(g_ready, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
            stack.Push(tmp[i]);

        for (int i = 0; i < TEST_CNT; i++)
            tmp[i] = stack.Pop();
    }

    for (int i = 0; i < TEST_CNT; i++)
        if (tmp[i] != (int)arg) __debugbreak();

    long ret = InterlockedIncrement(&g_completeCnt);
    if (ret == THREAD_CNT)
    {
        // Print Out Test Result Here
        SetEvent(g_allComplete);
    }
    else
    {
        WaitForSingleObject(g_allComplete, INFINITE);
    }
    return 0;
}

unsigned __stdcall QueueTestThread(void* arg);
void QueueTest()
{
    ResetEvent(g_ready);
    ResetEvent(g_allComplete);
    g_completeCnt = 0;

    HANDLE threads[THREAD_CNT];
    for (int i = 0; i < THREAD_CNT; i++)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, QueueTestThread, (void*)(i + 1), 0, nullptr);
        if (threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(g_ready);
    WaitForMultipleObjects(THREAD_CNT, threads, true, INFINITE);
    ::printf("Queue Test Complete!\n");
}

unsigned __stdcall QueueTestThread(void* arg)
{
    int tmp[TEST_CNT];
    for (int i = 0; i < TEST_CNT; i++)
        tmp[i] = (int)arg;
    CLockFreeQueue<int> queue;
    WaitForSingleObject(g_ready, INFINITE);

    for (int j = 0; j < LOOP_CNT; j++)
    {
        for (int i = 0; i < TEST_CNT; i++)
            queue.Enqueue(tmp[i]);

        for (int i = 0; i < TEST_CNT; i++)
            tmp[i] = queue.Dequeue();
    }

    for (int i = 0; i < TEST_CNT; i++)
        if (tmp[i] != (int)arg) __debugbreak();

    long ret = InterlockedIncrement(&g_completeCnt);
    if (ret == THREAD_CNT)
    {
        // Print Out Test Result Here
        SetEvent(g_allComplete);
    }
    else
    {
        WaitForSingleObject(g_allComplete, INFINITE);
    }

    return 0;
}