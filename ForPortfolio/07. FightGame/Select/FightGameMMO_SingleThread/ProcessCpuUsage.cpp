#include "ProcessCpuUsage.h"
#include <wchar.h>

CProcessCpuUsage::CProcessCpuUsage(HANDLE processHandle)
{
	if (processHandle == INVALID_HANDLE_VALUE)
		_processHandle = GetCurrentProcess();

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	_numOfProcessors = SystemInfo.dwNumberOfProcessors;

	_processLastTime.QuadPart = 0;
	_processLastKernel.QuadPart = 0;
	_processLastUser.QuadPart = 0;

	UpdateCpuTime();
}

void CProcessCpuUsage::PrintCpuData(void)
{
	UpdateCpuTime();

	wprintf(L"Process: %f\n", GetTotalTime());
	wprintf(L"ProcessKernel: %f\n", GetKernelTime());
	wprintf(L"ProcessUser: %f\n\n", GetUserTime());
}

void CProcessCpuUsage::UpdateCpuTime(void)
{
	// Get Process CPU Usage

	ULARGE_INTEGER none;
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;
	ULARGE_INTEGER nowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&nowTime);
	GetProcessTimes(_processHandle, (LPFILETIME)&none,
		(LPFILETIME)&none, (LPFILETIME)&kernel, (LPFILETIME)&user);

	ULONGLONG timeDiff = nowTime.QuadPart - _processLastTime.QuadPart;
	ULONGLONG kernelDiff = kernel.QuadPart - _processLastKernel.QuadPart;
	ULONGLONG userDiff = user.QuadPart - _processLastUser.QuadPart;
	ULONGLONG total = kernelDiff + userDiff;

	_processTotal = (float)(total / (double)_numOfProcessors / (double)timeDiff * 100.0f);
	_processKernel = (float)(kernelDiff / (double)_numOfProcessors / (double)timeDiff * 100.0f);
	_processUser = (float)(userDiff / (double)_numOfProcessors / (double)timeDiff * 100.0f);

	_processLastTime = nowTime;
	_processLastKernel = kernel;
	_processLastUser = user;
}
