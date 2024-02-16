#include "CLockFreeStack.h"
#include <process.h>  
#include <iostream>
#pragma comment(lib, "winmm.lib")

#define dfTHREAD_CNT 64
#define dfTEST_CNT 1000000

char input = 'a';
CLockFreeStack<char> g_Stack;

unsigned __stdcall TestThread(void* arg)
{
    for (int i = 0; i < dfTEST_CNT; i++)
    {
        g_Stack.Pop();
    }

    return 0;
}

int main()
{
    for (int i = 0; i < dfTHREAD_CNT * dfTEST_CNT; i++)
    {
        g_Stack.Push(input);
    }

    HANDLE threads[dfTHREAD_CNT];
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, TestThread, nullptr, 0, nullptr);
        if (threads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }

    WaitForMultipleObjects(dfTHREAD_CNT, threads, true, INFINITE);

}
