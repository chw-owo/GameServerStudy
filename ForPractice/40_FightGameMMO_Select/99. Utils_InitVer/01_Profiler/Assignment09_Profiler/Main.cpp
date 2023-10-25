#include <windows.h>
#include "ProfileForSingleThread.h"

int main()
{
	for (int i = 0; i < 10; i++)
	{
		PRO_BEGIN(L"Sleep 10");
		Sleep(10);
		PRO_END(L"Sleep 10");
	}

	PRO_OUT(L"output.txt");
	return 0;
}
