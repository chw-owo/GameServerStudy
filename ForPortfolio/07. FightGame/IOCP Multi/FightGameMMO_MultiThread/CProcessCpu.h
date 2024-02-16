#pragma once

#define __CPU_USGAE_H__
#ifndef __CPU_USAGE_H__

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif


#include <windows.h>

class CProcessCpu
{
public:
	CProcessCpu(HANDLE hProcess = INVALID_HANDLE_VALUE);
	void PrintCpuData(void);
	void UpdateCpuTime(void);
	float GetTotalTime(void) { return _fProcessTotal; }
	float GetUserTime(void) { return _fProcessUser; }
	float GetKernelTime(void) { return _fProcessKernel; }

private:
	HANDLE _hProcess;
	int _iNumOfProcessors;
	float _fProcessTotal = 0.0f;
	float _fProcessUser = 0.0f;
	float _fProcessKernel = 0.0f;
	ULARGE_INTEGER _ftProcess_LastTime;
	ULARGE_INTEGER _ftProcess_LastKernel;
	ULARGE_INTEGER _ftProcess_LastUser;
};

#endif