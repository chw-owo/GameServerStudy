#include "Main.h"

int wmain(int argc, wchar_t* argv[])
{
	if (!NetworkInit()) return;

	while (1)
	{
		if (!NetworkProc()) break;
	}

	NetworkTerminate();
	return 0;
}
