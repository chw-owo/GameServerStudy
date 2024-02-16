#include "CMonitorManager.h"

CMonitorManager::CMonitorManager(bool monitorTotal)
{
	_hProcess = GetCurrentProcess();
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	_iNumOfProcessors = SystemInfo.dwNumberOfProcessors;

	_ftTotalCPU_LastIdle.QuadPart = 0;
	_ftTotalCPU_LastKernel.QuadPart = 0;
	_ftTotalCPU_LastUser.QuadPart = 0;

	_ftProcess_LastTime.QuadPart = 0;
	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastUser.QuadPart = 0;
}

CMonitorManager::~CMonitorManager()
{
}

bool CMonitorManager::Initialize()
{
	return false;
}

void CMonitorManager::Terminate()
{
}

void CMonitorManager::UpdateProcessCPUTime()
{
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

	_fProcessCPU = (float)(Total / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernelCPU = (float)(KernelDiff / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUserCPU = (float)(UserDiff / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);

	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;
}

long CMonitorManager::GetProcessOnOff()
{
	return 0;
}

long CMonitorManager::GetProcessMemory()
{
	return 0;
}

void CMonitorManager::UpdateTotalCPUTime()
{
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false) return;

	ULONGLONG IdleDiff = Idle.QuadPart - _ftTotalCPU_LastIdle.QuadPart;
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftTotalCPU_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftTotalCPU_LastUser.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;

	if (Total == 0)
	{
		_fTotalCPU = 0.0f;
		_fTotalKernelCPU = 0.0f;
		_fTotalUserCPU = 0.0f;
	}
	else
	{
		_fTotalCPU = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_fTotalKernelCPU = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		_fTotalUserCPU = (float)((double)UserDiff / Total * 100.0f);
	}

	_ftTotalCPU_LastIdle = Idle;
	_ftTotalCPU_LastKernel = Kernel;
	_ftTotalCPU_LastUser = User;
}

long CMonitorManager::GetTotalNonpaged()
{
	return 0;
}

long CMonitorManager::GetTotalUsableMemory()
{
	return 0;
}

long CMonitorManager::GetTotalRecv()
{
	return 0;
}

long CMonitorManager::GetTotalSend()
{
	return 0;
}
