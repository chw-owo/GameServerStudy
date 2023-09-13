#pragma once

#define dfTHREAD_MAX 14
class DebugData2
{
	DebugData2();

	int _ns;
	int _line;

};
extern DebugData2 g_ArrayForDebug[dfTHREAD_MAX];

