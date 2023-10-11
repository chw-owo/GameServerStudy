#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#define __CPU_USGAE_H__
#ifndef __CPU_USAGE_H__

#include <windows.h>

class CProcessorCpuUsage
{
public:
	CProcessorCpuUsage();
	void PrintCpuData(void);
	void UpdateCpuTime(void);
	float GetTotalTime(void) { return _processorTotal; }
	float GetUserTime(void) { return _processorUser; }
	float GetKernelTime(void) { return _processorKernel; }

private:
	float _processorTotal = 0.0f;
	float _processorUser = 0.0f;
	float _processorKernel = 0.0f;
	ULARGE_INTEGER _processorLastIdle;
	ULARGE_INTEGER _processorLastKernel;
	ULARGE_INTEGER _processorLastUser;
};

#endif