#include "ProcessorCpuUsage.h"
#include <wchar.h>

CProcessorCpuUsage::CProcessorCpuUsage()
{
	_processorLastIdle.QuadPart = 0;
	_processorLastKernel.QuadPart = 0;
	_processorLastUser.QuadPart = 0;
	UpdateCpuTime();
}

void CProcessorCpuUsage::PrintCpuData(void)
{
	UpdateCpuTime();

	wprintf(L"Processor: %f\n", GetTotalTime());
	wprintf(L"ProcessorKernel: %f\n", GetKernelTime());
	wprintf(L"ProcessorUser: %f\n\n", GetUserTime());
}

void CProcessorCpuUsage::UpdateCpuTime(void)
{
	// Get Processor CPU Usage

	ULARGE_INTEGER idle;
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;
	if (GetSystemTimes((PFILETIME)&idle, (PFILETIME)&kernel, (PFILETIME)&user) == false) return;

	ULONGLONG idleDiff = idle.QuadPart - _processorLastIdle.QuadPart;
	ULONGLONG kernelDiff = kernel.QuadPart - _processorLastKernel.QuadPart;
	ULONGLONG userDiff = user.QuadPart - _processorLastUser.QuadPart;
	ULONGLONG total = kernelDiff + userDiff;

	if (total == 0)
	{
		_processorTotal = 0.0f;
		_processorKernel = 0.0f;
		_processorUser = 0.0f;
	}
	else
	{
		_processorTotal = (float)((double)(total - idleDiff) / total * 100.0f);
		_processorKernel = (float)((double)(kernelDiff - idleDiff) / total * 100.0f);
		_processorUser = (float)((double)userDiff / total * 100.0f);
	}

	_processorLastIdle = idle;
	_processorLastKernel = kernel;
	_processorLastUser = user;
}
