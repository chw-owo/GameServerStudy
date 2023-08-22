#include "CProcessorCpuUsage.h"
#include "CProcessCpuUsage.h"
#include <windows.h>
#include <wchar.h>

int main()
{
	CProcessorCpuUsage ProcessorCPUTime;
	CProcessCpuUsage ProcessCPUTime;

	while (1)
	{
		ProcessorCPUTime.UpdateCpuTime();
		ProcessCPUTime.UpdateCpuTime();

		wprintf(L"Processor: %f\n", ProcessorCPUTime.GetTotalTime());
		wprintf(L"ProcessorKernel: %f\n", ProcessorCPUTime.GetKernelTime());
		wprintf(L"ProcessorUser: %f\n\n", ProcessorCPUTime.GetUserTime());

		wprintf(L"Process: %f\n", ProcessCPUTime.GetTotalTime());
		wprintf(L"ProcessKernel: %f\n", ProcessCPUTime.GetKernelTime());
		wprintf(L"ProcessUser: %f\n\n", ProcessCPUTime.GetUserTime());

		wprintf(L"=================================\n\n");

		Sleep(1000);
	}
}
