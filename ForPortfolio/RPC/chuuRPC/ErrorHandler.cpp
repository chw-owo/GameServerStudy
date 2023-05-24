#include "ErrorHandler.h"
void AssertCrash()
{
	char* pNull = nullptr;
	*pNull = 0xee;
}