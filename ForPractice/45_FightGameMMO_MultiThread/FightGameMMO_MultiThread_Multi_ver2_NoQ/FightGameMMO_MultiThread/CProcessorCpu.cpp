#include "CProcessorCpu.h"
#include <wchar.h>

CProcessorCpu::CProcessorCpu()
{
	_ftProcessor_LastIdle.QuadPart = 0;
	_ftProcessor_LastKernel.QuadPart = 0;
	_ftProcessor_LastUser.QuadPart = 0;
	UpdateCpuTime();
}

void CProcessorCpu::PrintCpuData(void)
{
	UpdateCpuTime();

	::wprintf(L"Processor: %f\n", GetTotalTime());
	::wprintf(L"ProcessorKernel: %f\n", GetKernelTime());
	::wprintf(L"ProcessorUser: %f\n\n", GetUserTime());

}

void CProcessorCpu::UpdateCpuTime(void)
{
	// Get Processor CPU Usage

	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false) return;

	ULONGLONG IdleDiff = Idle.QuadPart - _ftProcessor_LastIdle.QuadPart;
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcessor_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftProcessor_LastUser.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;

	if (Total == 0)
	{
		_fProcessorTotal = 0.0f;
		_fProcessorKernel = 0.0f;
		_fProcessorUser = 0.0f;
	}
	else
	{
		_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
	}

	_ftProcessor_LastIdle = Idle;
	_ftProcessor_LastKernel = Kernel;
	_ftProcessor_LastUser = User;
}
