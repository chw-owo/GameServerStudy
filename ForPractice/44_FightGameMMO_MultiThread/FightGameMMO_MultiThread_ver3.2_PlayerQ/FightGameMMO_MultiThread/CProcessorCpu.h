#pragma once

#define __CPU_USGAE_H__
#ifndef __CPU_USAGE_H__

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

class CProcessorCpu
{
public:
	CProcessorCpu();
	void PrintCpuData(void);
	void UpdateCpuTime(void);
	float GetTotalTime(void) { return _fProcessorTotal; }
	float GetUserTime(void) { return _fProcessorUser; }
	float GetKernelTime(void) { return _fProcessorKernel; }

private:
	float _fProcessorTotal = 0.0f;
	float _fProcessorUser = 0.0f;
	float _fProcessorKernel = 0.0f;
	ULARGE_INTEGER _ftProcessor_LastIdle;
	ULARGE_INTEGER _ftProcessor_LastKernel;
	ULARGE_INTEGER _ftProcessor_LastUser;
};

#endif