#include "CProcessCpuUsage.h"
#include <wchar.h>

CProcessCpuUsage::CProcessCpuUsage(HANDLE hProcess)
{
	if (hProcess == INVALID_HANDLE_VALUE)
		_hProcess = GetCurrentProcess();

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	_iNumOfProcessors = SystemInfo.dwNumberOfProcessors;

	_ftProcess_LastTime.QuadPart = 0;
	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastUser.QuadPart = 0;

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

	ULARGE_INTEGER None;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	ULARGE_INTEGER NowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);
	GetProcessTimes(_hProcess, (LPFILETIME)&None,
		(LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

	ULONGLONG TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;

	_fProcessTotal = (float)(Total / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernel = (float)(KernelDiff / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUser = (float)(UserDiff / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);

	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;
}
