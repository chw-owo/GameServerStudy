#include "CLockFreeQueue.h"
#include "CLockFreeStack.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>

#define TEST_CNT 3
#define LOOP_CNT 100000
#define STRUCT_CNT 3
#define THREAD_CNT 16

HANDLE g_ready;
HANDLE g_allComplete;
CLockFreeStack<int>* stacks[STRUCT_CNT];
CLockFreeQueue<int>* queues[STRUCT_CNT];

long g_completeCnt = 0;
void Setting();
void StackTest();
void QueueTest();

int main()
{ 
   Setting();
   // StackTest();
   QueueTest();
   __debugbreak();
}

void Setting()
{
    for (int i = 0; i < STRUCT_CNT; i++)
    {
        stacks[i] = new CLockFreeStack<int>;
        queues[i] = new CLockFreeQueue<int>;
    }

    g_ready = CreateEvent(NULL, true, false, nullptr);
    if (g_ready == NULL) __debugbreak();
    g_allComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_ready == NULL) __debugbreak();
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
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, StackTestThread, (void*)i, 0, nullptr);
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
    int threadIdx = (int)arg;
    WaitForSingleObject(g_ready, INFINITE);

    for (int i = 0; i < LOOP_CNT; i++)
    {
        for (int k = 0; k < STRUCT_CNT; k++)
        {
            for (int j = 0; j < TEST_CNT; j++)
            {
                stacks[k]->Push(k + 1);
            }
        }

        for (int j = 0; j < TEST_CNT; j++)
        {
            for (int k = 0; k < STRUCT_CNT; k++)
            {
                int ret = stacks[k]->Pop();
                if (ret != k + 1 && ret != 0) __debugbreak();
            }
        }
    }

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
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, QueueTestThread, (void*)i, 0, nullptr);
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
    int threadIdx = (int)arg;
    WaitForSingleObject(g_ready, INFINITE);

    for (int i = 0; i < LOOP_CNT; i++)
    {    
        for (int k = 0; k < STRUCT_CNT; k++)
        {
            for (int j = 0; j < TEST_CNT; j++)
            {
                queues[k]->Enqueue(k + 1);
            }
        }

        for (int j = 0; j < TEST_CNT; j++)
        {
            for (int k = 0; k < STRUCT_CNT; k++)
            {
                int ret = queues[k]->Dequeue();
                if (ret != k + 1 && ret != 0) __debugbreak();
                // 아~~~ 드디어 에러 난다~~~
            }
        }
    }

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