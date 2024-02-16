#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#define __CPU_USGAE_H__
#ifndef __CPU_USAGE_H__

#include <windows.h>

class CProcessCpuUsage
{
public:
	CProcessCpuUsage(HANDLE processHandle = INVALID_HANDLE_VALUE);
	void PrintCpuData(void);
	void UpdateCpuTime(void);
	float GetTotalTime(void) { return _processTotal; }
	float GetUserTime(void) { return _processUser; }
	float GetKernelTime(void) { return _processKernel; }

private:
	HANDLE _processHandle;
	int _numOfProcessors;
	float _processTotal = 0.0f;
	float _processUser = 0.0f;
	float _processKernel = 0.0f;
	ULARGE_INTEGER _processLastTime;
	ULARGE_INTEGER _processLastKernel;
	ULARGE_INTEGER _processLastUser;
};

#endif