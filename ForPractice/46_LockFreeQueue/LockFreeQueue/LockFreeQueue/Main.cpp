#include "LockFreeQueueTester.h"
#include <windows.h>

int main()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCount = (int)si.dwNumberOfProcessors - 2;

	CLockFreeQueueTester* pTester = new CLockFreeQueueTester;
	pTester->StartTest(threadCount);

	return 0;
}
