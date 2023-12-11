#include "CLockFreeQueue.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>

// #define __MONITOR
#define TEST_CNT 3
#define QUEUE_CNT 2
#define THREAD_CNT 4
#define LOOP_CNT 1000000

HANDLE g_ready;
HANDLE g_allComplete;
long g_completeCnt = 0;
int enqTPS[THREAD_CNT] = { 0, };
int deqTPS[THREAD_CNT] = { 0, };
CLockFreeQueue<int>* queues[QUEUE_CNT];

void Setting();
void QueueTest();
unsigned __stdcall QueueTestThread(void* arg);

int main()
{
    Setting();
    QueueTest();
    for (int i = 0; i < QUEUE_CNT; i++)
    {
        delete(queues[i]);
    }
    ::printf("Queue Test Complete!\n");
    for (;;);
}

void Setting()
{
    for (int i = 0; i < QUEUE_CNT; i++)
    {
        queues[i] = new CLockFreeQueue<int>;
    }

    g_ready = CreateEvent(NULL, true, false, nullptr);
    if (g_ready == NULL) __debugbreak();
    g_allComplete = CreateEvent(NULL, true, false, nullptr);
    if (g_ready == NULL) __debugbreak();
}

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

#ifdef __MONITOR

    int sum[QUEUE_CNT] = { 0, };
    for (;;)
    {
        Sleep(1000);

        for (int i = 0; i < THREAD_CNT; i++)
        {
            ::printf("%d - %d\n", enqTPS[i], deqTPS[i]);
            enqTPS[i] = 0;
            deqTPS[i] = 0;
        }

        ::printf("\n");
    }
#endif

    WaitForMultipleObjects(THREAD_CNT, threads, true, INFINITE);

}

unsigned __stdcall QueueTestThread(void* arg)
{
    int threadIdx = (int)arg;
    WaitForSingleObject(g_ready, INFINITE);

    for (;;) // (int i = 0; i < LOOP_CNT; i++)
    {
        for (int k = 0; k < QUEUE_CNT; k++)
        {
            for (int j = 0; j < TEST_CNT; j++)
            {
                queues[k]->Enqueue(k + 1);
                enqTPS[threadIdx]++;
            }
        }

        for (int j = 0; j < TEST_CNT; j++)
        {
            for (int k = 0; k < QUEUE_CNT; k++)
            {
                int ret = queues[k]->Dequeue();
                if (ret != 0 && ret != k + 1) __debugbreak();
                if (ret != 0) deqTPS[threadIdx]++;
            }
        }

        if (GetAsyncKeyState(VK_SPACE)) break;
    }

    long ret = InterlockedIncrement(&g_completeCnt);
    if (ret == THREAD_CNT) SetEvent(g_allComplete);
    else  WaitForSingleObject(g_allComplete, INFINITE);

    return 0;
}
