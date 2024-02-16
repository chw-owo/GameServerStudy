#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <windows.h>

class CData
{
public:
	long _val = -1;
	long _timestamp = -1;

	long _sum = 0;
	long _call = 0;
	long _min = LONG_MAX;
	long _max = LONG_MIN;
};