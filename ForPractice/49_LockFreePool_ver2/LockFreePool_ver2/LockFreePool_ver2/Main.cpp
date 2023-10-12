#include "CLockFreePoolTester.h"
#include <windows.h>

int main()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCount = (int)si.dwNumberOfProcessors - 2;

	CLockFreePoolTester* pTester = new CLockFreePoolTester;
	pTester->StartTest(threadCount);

	return 0;
}
