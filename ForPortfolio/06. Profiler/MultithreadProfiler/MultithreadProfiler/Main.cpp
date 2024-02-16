#include "CProfiler.h"
#include <Windows.h>
#include <process.h>

static unsigned int WINAPI Test(void* arg);

int main()
{
	HANDLE _threads[8];
	for (int i = 0; i < 8; i++)
	{
		_threads[i] = (HANDLE)_beginthreadex(NULL, 0, Test, nullptr, 0, nullptr);
	}
    WaitForMultipleObjects(8, _threads, true, INFINITE);
    PRO_OUT(L"Hey.txt");
}

static unsigned int WINAPI Test(void* arg)
{
    int a = 0;
    for (int j = 0; j < 100; j++)
    {
        PRO_BEGIN("Hey");
        for (int i = 0; i < 1000; i++)
        {
            a++;
        }
        PRO_END("Hey");
    }

    return 0;
}
